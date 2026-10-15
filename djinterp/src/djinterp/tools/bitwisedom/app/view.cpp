/*******************************************************************************
* bitwisedom [app]                                                      view.cpp
*
* The interface, as uxoxo templates built from the model.
*   build_ui() is a pure function of the model: it reads, never writes. The
* window is a menu bar and a toolbar over three panels -- the tree of rows,
* the bit grid, the inspector -- with a tabbed panel beneath them, the
* panels divided by splitters. Only the front tab's content is built; the
* rest wait until they come to the front.
*   Every input that sits below something that can appear or vanish carries
* a key, so an edit in progress keeps its focus when, say, an error line
* appears above it. Every table carries one too: ImGui remembers a table's
* column widths by its identity, between frames and between runs.
*   A row is renamed in place: its tree node becomes an input when the
* model says it is being renamed there, and the grid draws its own. The
* context menu is one popup, built from what the model says it is about. A
* closed group's rows are not built; Expand all and Collapse all, under the
* tree's add buttons, on the View menu and on a group's menu, open and
* close every group at once.
*   The panels lie flush, divided by hairlines that are the splitters. The
* tabbed panel is shown as the model says: docked under the grid, minimized
* to a strip of its tabs, minimized until the mouse rests on that strip and
* then laid over the grid, floating in a window of its own, or not at all.
*
* path:      /src/djinterp/tools/bitwisedom/app/view.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./view.hpp"  // corresponding header

// std
#include <algorithm>  // std::find, std::max
#include <cstddef>    // std::size_t
#include <string>     // std::string
#include <vector>     // std::vector
// uxoxo
#include <uxoxo/element_button.hpp>        // button
#include <uxoxo/element_color_field.hpp>   // color_field
#include <uxoxo/element_column.hpp>        // column
#include <uxoxo/element_combo.hpp>         // combo
#include <uxoxo/element_label.hpp>         // label
#include <uxoxo/element_menu.hpp>          // menu_bar, menu, menu_item
#include <uxoxo/element_number_field.hpp>  // int_field, slider
#include <uxoxo/element_panel.hpp>         // panel
#include <uxoxo/element_popup.hpp>         // popup
#include <uxoxo/element_row.hpp>           // row
#include <uxoxo/element_section.hpp>       // section
#include <uxoxo/element_selectable.hpp>    // selectable
#include <uxoxo/element_separator.hpp>     // separator
#include <uxoxo/element_splitter.hpp>      // splitter
#include <uxoxo/element_table.hpp>         // table, table_row
#include <uxoxo/element_tabs.hpp>          // tabs, tab
#include <uxoxo/element_text_field.hpp>    // text_field
#include <uxoxo/element_toggle.hpp>        // toggle
#include <uxoxo/element_tree_node.hpp>     // tree_node
#include <uxoxo/element_window.hpp>        // window
// imgui
#include <imgui.h>  // the frame's metrics, for the strip's height
// bitwisedom
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_list
#include "./app.hpp"                       // range_choices, op_targets
#include "./attrs.hpp"                     // attrs
#include "./elements.hpp"                  // bit_grid, flow_view, ...
#include "./settings.hpp"                  // settings
#include "./theme.hpp"                     // ui_scale


namespace bitwisedom
{
namespace gui
{

namespace
{

using namespace ::uxoxo::component;
using ::uxoxo::element_template;
using ::uxoxo::option_set;
using templates = std::vector<element_template>;

// hrow
//   uxoxo's row container. bitwisedom has a type called row, which hides
// the function inside this namespace; this name does not collide.
element_template
hrow(
    std::vector<element_template> _items,
    int                           _spacing = 1,
    option_set                    _attrs   = option_set{}
)
{
    return ::uxoxo::component::row(std::move(_items),
                                   _spacing,
                                   std::move(_attrs));
}

// SPLIT
//   constant: a splitter's thickness, in pixels: the strip that takes the
// mouse, with a hairline down its middle.
const double SPLIT = 5.0;

// KIND_NAMES / ENCODING_NAMES / OP_NAMES
//   the choices of the inspector's and the operations panel's combos.
const std::vector<std::string> KIND_NAMES = {
    "bit", "range", "value in a range", "literal", "expression", "macro" };

// ------------------------------------------------------------------ helpers

std::string
hex(
    u64 _value,
    int _width
)
{
    return "0x" + group_digits(
        format_hex(_value, (_width + 3) / 4, true), 2);
}

// brief
//   a value in a line: in hex, where an integer holds it; else as its
// runs, the first few named.
std::string
brief(
    const wide& _value,
    int         _width
)
{
    return is_wide(_width) ? runs_text(_value, 3)
                           : hex(_value.low(), _width);
}

// brief
//   a row's value in a line: a wide sheet's macro is C's integer, and its
// other symbols are sets of bits.
std::string
brief(
    const resolved_row& _row,
    int                 _width
)
{
    if ( (is_wide(_width)) &&
         (_row.def == def_kind::macro) )
    {
        return "0x" + group_digits(format_hex(_row.value, 0, true), 2);
    }

    return is_wide(_width) ? brief(_row.bits, _width)
                           : hex(_row.value, _width);
}

element_template
muted(
    const std::string& _text
)
{
    return label(_text, attrs({ { "tone", "muted" }, { "wrap", true } }));
}

element_template
coloured(
    const std::string& _text,
    rgba               _colour
)
{
    const double r = channel(_colour, 0);
    const double g = channel(_colour, 1);
    const double b = channel(_colour, 2);

    return label(_text, attrs({ { "r", r },
                                { "g", g },
                                { "b", b },
                                { "a", 1.0 } }));
}

// level_label
//   text in the colour of an issue's severity: errors red, warnings amber,
// notes muted.
element_template
level_label(
    const std::string& _text,
    severity           _level,
    bool               _wrap
)
{
    if (_level == severity::error)
    {
        return label(_text, attrs({ { "tone", "error" }, { "wrap", _wrap } }));
    }

    if (_level == severity::warning)
    {
        return label(_text, attrs({ { "r", 0.96 },
                                    { "g", 0.68 },
                                    { "b", 0.22 },
                                    { "a", 1.0 },
                                    { "wrap", _wrap } }));
    }

    return label(_text, attrs({ { "tone", "muted" }, { "wrap", _wrap } }));
}

// action_button
//   a button that posts an action; _extra overlays its attributes.
element_template
action_button(
    const std::string& _text,
    const std::string& _action,
    option_set         _extra = option_set{},
    bool               _enabled = true
)
{
    option_set made = attrs({ { "action", _action } });

    return button(label(_text),
                  _enabled,
                  ::djinterp::overlay(made, _extra));
}

// form
//   a two-column table: labels on the left, controls on the right.
element_template
form(
    const std::string& _id,
    templates          _rows,
    double             _label_width = 128.0
)
{
    // the label column can be dragged wider, and keeps the width
    return table(_id,
                 { "", "" },
                 std::move(_rows),
                 attrs({ { "borders", "none" },
                         { "row_bg", false },
                         { "resizable", true },
                         { "key", "form." + _id },
                         { "widths", format_dec(static_cast<u64>(
                                         _label_width)) + "\n0" } }));
}

// shared_form
//   a form whose two columns share its width, two parts to three: for a
// panel that may be narrow, where a fixed label column would leave the
// controls no room.
element_template
shared_form(
    const std::string& _id,
    templates          _rows
)
{
    return table("shared." + _id,
                 { "", "" },
                 std::move(_rows),
                 attrs({ { "borders", "none" },
                         { "row_bg", false },
                         { "resizable", true },
                         { "key", "form.shared." + _id },
                         { "widths", "2*\n3*" } }));
}

element_template
form_row(
    const std::string& _label,
    element_template   _control
)
{
    return table_row({ label(_label), std::move(_control) });
}

// setting_control
//   the control that edits one registered setting.
element_template
setting_control(
    const model&       _model,
    const std::string& _path,
    option_set         _extra = option_set{}
)
{
    const setting* found = find_setting(_path);

    if (found == nullptr)
    {
        return muted("(no setting " + _path + ")");
    }

    // colours and the grid read as they are shown: an automatic colour as
    // the colour the theme and scheme give it
    double value = 0.0;

    if ( (_path.compare(0, 8, "colours.") == 0) ||
         (_path.compare(0, 5, "grid.") == 0) )
    {
        sheet shown;
        shown.colours = _model.shown.colours;
        shown.grid    = _model.shown.grid;

        value = found->read(shown);

        // the palette itself says whether it is the scheme's
        if (_path == "colours.palette")
        {
            value = found->read(_model.doc);
        }
    }
    else
    {
        value = found->read(_model.doc);
    }

    option_set   base  = attrs({ { "target", _path },
                                 { "key", _path },
                                 { "tooltip", found->help } });

    base = ::djinterp::overlay(base, _extra);

    switch (found->kind)
    {
        case setting_kind::flag:
            return toggle(found->label, value != 0.0, "set", base);

        case setting_kind::integer:
            return int_field(static_cast<long>(value),
                             "set",
                             ::djinterp::overlay(
                                 attrs({ { "min", static_cast<long>(
                                                      found->low) },
                                         { "max", static_cast<long>(
                                                      found->high) },
                                         { "drag", true },
                                         { "width", 1.0 } }),
                                 base));

        case setting_kind::real:
            return slider(value,
                          found->low,
                          found->high,
                          "set",
                          ::djinterp::overlay(
                              attrs({ { "format", (found->high <= 1.0)
                                                      ? "%.2f" : "%.1f" },
                                      { "width", 1.0 } }),
                              base));

        case setting_kind::colour:
        {
            const rgba colour = static_cast<rgba>(value);

            return color_field(channel(colour, 0),
                               channel(colour, 1),
                               channel(colour, 2),
                               channel(colour, 3),
                               "set",
                               base);
        }

        case setting_kind::choice:
            return combo(found->choices,
                         static_cast<long>(value),
                         "set",
                         ::djinterp::overlay(attrs({ { "width", 1.0 } }),
                                             base));

        case setting_kind::text:
            return text_field(found->read_text(_model.doc), "set", base);
    }

    return muted(_path);
}

// setting_rows
//   form rows for registered settings, by path; a flag's label is its own.
templates
setting_rows(
    const model&                    _model,
    const std::vector<std::string>& _paths
)
{
    templates rows;

    for (const std::string& path : _paths)
    {
        const setting* found = find_setting(path);

        if (found == nullptr)
        {
            continue;
        }

        if (found->kind == setting_kind::flag)
        {
            rows.push_back(table_row({ label(""),
                                       setting_control(_model, path) }));
        }
        else
        {
            rows.push_back(form_row(found->label,
                                    setting_control(_model, path)));
        }
    }

    return rows;
}

// openness
//   struct: whether any of a sheet's groups is closed, and whether any is
// open -- what Expand all and Collapse all would change.
struct openness
{
    bool closed = false;
    bool open   = false;
};

openness
openness_of(
    const sheet& _sheet
)
{
    openness found;

    walk(_sheet,
         [&found](const row& _row, const row&, int)
         {
             if (can_contain(_row) && !_row.children.empty())
             {
                 found.closed = ( (found.closed) || (_row.collapsed) );
                 found.open   = ( (found.open) || (!_row.collapsed) );
             }

             return;
         });

    return found;
}

// --------------------------------------------------------------- the menus

element_template
menus(
    const model& _model
)
{
    // the examples, on their shelves in the order the shelves first
    // appear, each with its size beside it
    const std::vector<example_info>& list = example_list();
    std::vector<std::string>         shelves;
    templates                        examples;

    for (const example_info& each : list)
    {
        if (std::find(shelves.begin(), shelves.end(), each.shelf) ==
            shelves.end())
        {
            shelves.push_back(each.shelf);
        }
    }

    for (const std::string& shelf : shelves)
    {
        templates items;

        for (std::size_t i = 0; i < list.size(); ++i)
        {
            if (list[i].shelf != shelf)
            {
                continue;
            }

            const int         width = list[i].width;
            const std::string size  =
                ((width % 8) == 0)
                    ? format_dec(static_cast<u64>(width / 8)) +
                          ((width == 8) ? " byte" : " bytes")
                    : format_dec(static_cast<u64>(width)) + " bits";

            items.push_back(menu_item(list[i].title, "file.example",
                                      attrs({ { "value",
                                                static_cast<long>(i) },
                                              { "shortcut", size } })));
        }

        examples.push_back(menu(shelf, std::move(items)));
    }

    // the Theme menu: the themes, then the colour schemes, each checked
    // when it is the window's
    const ui_state& ui = _model.ui;
    templates       looks;

    for (int i = 0; i < THEME_COUNT; ++i)
    {
        looks.push_back(menu_item(theme_name(i), "view.theme",
                                  attrs({ { "value", static_cast<long>(i) },
                                          { "checkable", true },
                                          { "checked", ui.theme == i } })));
    }

    looks.push_back(separator("Colour scheme"));

    for (int i = 0; i < SCHEME_COUNT; ++i)
    {
        looks.push_back(menu_item(scheme_name(i), "view.scheme",
                                  attrs({ { "value", static_cast<long>(i) },
                                          { "checkable", true },
                                          { "checked", ui.scheme == i } })));
    }

    // whether the sheet chooses any colour the scheme would
    const colour_settings& own   = _model.doc.colours;
    const grid_settings&   lines = _model.doc.grid;
    const bool             owned =
        ( (own.palette >= 0) || (own.saturation >= 0.0f) ||
          (own.value >= 0.0f) || (own.accent != 0) ||
          (own.empty_cell != 0) || (own.collision != 0) ||
          (own.probe != 0) || (own.selection != 0) || (own.hover != 0) ||
          (own.ghost != 0) || (lines.minor.colour != 0) ||
          (lines.major.colour != 0) || (lines.rows.colour != 0) ||
          (lines.groups.colour != 0) || (lines.segments.colour != 0) ||
          (lines.border.colour != 0) );

    looks.push_back(separator());
    looks.push_back(menu_item("The sheet's colours...", "tab.show",
                              attrs({ { "value", 5L } })));
    looks.push_back(menu_item("Give the sheet the scheme's colours",
                              "colours.automatic",
                              attrs({ { "enabled", owned } })));

    // the bottom panel's ways of being shown
    const char* const modes[PANEL_MODE_COUNT] = {
        "Docked below the grid",
        "Minimized to its tabs",
        "Minimized until the mouse is over it",
        "Floating in its own window",
        "Hidden" };
    templates panel_modes;

    for (int i = 0; i < PANEL_MODE_COUNT; ++i)
    {
        panel_modes.push_back(menu_item(
            modes[i], "panel.mode",
            attrs({ { "value", static_cast<long>(i) },
                    { "checkable", true },
                    { "checked", static_cast<int>(ui.panel) == i } })));
    }

    panel_modes.push_back(separator());
    panel_modes.push_back(menu_item(
        (ui.panel == panel_mode::hidden) ? "Show it" : "Hide it",
        "panel.toggle",
        attrs({ { "shortcut", "Ctrl+J" } })));

    // view_toggle
    //   a checkable menu item over a flag setting.
    auto view_toggle = [&_model](const char* _text,
                                 const char* _path,
                                 const char* _shortcut)
    {
        const setting* found = find_setting(_path);
        const bool     on    = ( (found != nullptr) &&
                                 (found->read(_model.doc) != 0.0) );

        return menu_item(_text, "set",
                         attrs({ { "target", _path },
                                 { "value", !on },
                                 { "checkable", true },
                                 { "checked", on },
                                 { "shortcut", _shortcut } }));
    };

    const bool     selected = !_model.ui.selection.empty();
    const long     first    = selected ? static_cast<long>(
                                             _model.ui.selection.front())
                                       : 0L;
    const openness groups   = openness_of(_model.doc);

    return menu_bar({
        menu("File", {
            menu_item("New", "file.new"),
            menu_item("Open sheet, bitfield or header", "file.open",
                      attrs({ { "shortcut", "Ctrl+O" } })),
            menu_item("Save", "file.save", attrs({ { "shortcut", "Ctrl+S" } })),
            menu_item("Export header", "file.export",
                      attrs({ { "shortcut", "Ctrl+E" } })),
            menu_item("Export the bitfield as JSON", "file.export_bitfield"),
            menu_item("Export everything as JSON", "file.export_everything",
                      attrs({ { "shortcut", "Ctrl+Shift+E" } })),
            separator(),
            menu_item("New sheet from the clipboard (code or JSON)",
                      "code.new"),
            menu_item("Copy the whole sheet as code", "code.copy",
                      attrs({ { "value", 0L } })),
            menu_item("Copy the bitfield as JSON", "json.copy"),
            separator(),
            menu("Examples", std::move(examples)),
            separator(),
            menu_item("Quit", "app.quit") }),
        menu("Edit", {
            menu_item("Undo", "edit.undo",
                      attrs({ { "shortcut", "Ctrl+Z" },
                              { "enabled", _model.steps.can_undo() } })),
            menu_item("Redo", "edit.redo",
                      attrs({ { "shortcut", "Ctrl+Y" },
                              { "enabled", _model.steps.can_redo() } })),
            separator(),
            menu_item("Add bit", "edit.add",
                      attrs({ { "value", 0L }, { "shortcut", "Ctrl+B" } })),
            menu_item("Add range", "edit.add", attrs({ { "value", 1L } })),
            menu_item("Add value in a range", "edit.add",
                      attrs({ { "value", 2L } })),
            menu_item("Add literal", "edit.add", attrs({ { "value", 3L } })),
            menu_item("Add expression", "edit.add",
                      attrs({ { "value", 4L } })),
            menu_item("Add macro", "edit.add", attrs({ { "value", 5L } })),
            menu_item("Add group", "edit.add",
                      attrs({ { "value", -1L }, { "shortcut", "Ctrl+G" } })),
            separator(),
            menu_item("Rename", "rename.tree",
                      attrs({ { "shortcut", "F2" },
                              { "value", first },
                              { "enabled", selected } })),
            menu_item("Copy as code", "code.copy",
                      attrs({ { "shortcut", "Ctrl+Shift+C" },
                              { "value", first },
                              { "enabled", selected } })),
            menu_item("Paste code", "code.paste",
                      attrs({ { "shortcut", "Ctrl+Shift+V" },
                              { "value", first } })),
            separator(),
            menu_item("Duplicate", "edit.duplicate",
                      attrs({ { "shortcut", "Ctrl+D" },
                              { "enabled", selected } })),
            menu_item("Delete", "edit.delete",
                      attrs({ { "shortcut", "Del" },
                              { "enabled", selected } })),
            menu_item("Move up", "edit.move_up",
                      attrs({ { "shortcut", "Alt+Up" },
                              { "enabled", selected } })),
            menu_item("Move down", "edit.move_down",
                      attrs({ { "shortcut", "Alt+Down" },
                              { "enabled", selected } })) }),
        menu("View", {
            view_toggle("Grid lines", "grid.lines", "G"),
            view_toggle("Bit indices", "grid.show_indices", ""),
            view_toggle("Digits in cells", "grid.show_digits", ""),
            view_toggle("Names across ranges", "grid.show_labels", ""),
            view_toggle("Cell boxes", "grid.show_cell_boxes", ""),
            view_toggle("Probe row", "grid.show_probe", ""),
            view_toggle("Usage row", "grid.show_usage", ""),
            view_toggle("Macro names in the grid", "grid.full_names", ""),
            view_toggle("Abbreviate unused runs", "grid.abbreviate_unused",
                        ""),
            separator(),
            menu_item("Expand all groups", "tree.open_all",
                      attrs({ { "value", 0L },
                              { "shortcut", "Ctrl+Shift+Plus" },
                              { "enabled", groups.closed } })),
            menu_item("Collapse all groups", "tree.close_all",
                      attrs({ { "value", 0L },
                              { "shortcut", "Ctrl+Shift+Minus" },
                              { "enabled", groups.open } })),
            separator(),
            menu_item("Symbols panel", "view.symbols",
                      attrs({ { "value", !ui.show_left },
                              { "checkable", true },
                              { "checked", ui.show_left } })),
            menu_item("Inspector", "view.inspector",
                      attrs({ { "value", !ui.show_right },
                              { "checkable", true },
                              { "checked", ui.show_right } })),
            menu("Bottom panel", std::move(panel_modes)),
            separator(),
            menu_item("ImGui demo", "app.demo",
                      attrs({ { "checkable", true },
                              { "checked", _model.ui.demo } })) }),
        menu("Theme", std::move(looks)) });
}

element_template
toolbar(
    const model& _model
)
{
    const std::string title = _model.doc.title +
                              (_model.ui.dirty ? " *" : "");

    return hrow({
        label(title, attrs({ { "tone", "accent" } })),
        text_field(_model.ui.path, "file.path",
                   attrs({ { "width", 240.0 },
                           { "key", "path" },
                           { "placeholder", "sheet file" },
                           { "tooltip", "the sheet file Open and Save use; "
                                        "Export writes the header beside "
                                        "it. Open a C or C++ header, or "
                                        "drop one on the window, to "
                                        "import its #defines as a new "
                                        "sheet" } })),
        action_button("Open", "file.open"),
        action_button("Save", "file.save"),
        action_button("Export .h", "file.export",
                      attrs({ { "tooltip", "writes " +
                                           _model.doc.gen.file_name } })),
        label("|", attrs({ { "tone", "muted" } })),
        label("width"),
        int_field(_model.doc.width, "set",
                  attrs({ { "target", "sheet.width" },
                          { "min", 1L },
                          { "max", static_cast<long>(MAX_WIDTH) },
                          { "drag", true },
                          { "width", 64.0 },
                          { "key", "width" },
                          { "tooltip", "the sheet's width in bits: up to "
                                       "64, an integer; wider, a set of "
                                       "bits C holds as arrays of words, "
                                       "up to 65536" } })),
        setting_control(_model, "grid.lines"),
        label("|", attrs({ { "tone", "muted" } })),
        action_button("Undo", "edit.undo", option_set{},
                      _model.steps.can_undo()),
        action_button("Redo", "edit.redo", option_set{},
                      _model.steps.can_redo()),
        label("|", attrs({ { "tone", "muted" } })),
        label(_model.ui.status,
              attrs({ { "tone", _model.ui.status_error ? "error"
                                                       : "muted" } })) });
}

// ----------------------------------------------------------------- the tree

std::string
kind_tag(
    const row& _row
)
{
    if (_row.kind == row_kind::group)
    {
        return "";
    }

    switch (_row.def)
    {
        case def_kind::bit:         return "bit";
        case def_kind::range:       return "range";
        case def_kind::field_value: return "value";
        case def_kind::literal:     return "literal";
        case def_kind::expression:  return "expr";
        case def_kind::macro:       return "macro";
    }

    return "";
}

element_template
tree_row(
    const model& _model,
    const row&   _row
)
{
    templates           children;
    const resolved_row* resolved = _model.res.find(_row.id);
    const bool          group    = can_contain(_row);

    // a closed group's rows are not drawn, so they are not built: a sheet
    // of a thousand rows in closed groups builds the groups alone
    if ( (!group) ||
         (!_row.collapsed) )
    {
        for (const row& child : _row.children)
        {
            children.push_back(tree_row(_model, child));
        }
    }

    const rgba        colour = (resolved != nullptr) ? resolved->colour
                                                     : rgba(0);
    const std::string name   = _row.name.empty() ? std::string("(unnamed)")
                                                 : _row.name;
    const bool        broken = ( (resolved != nullptr) &&
                                 (!resolved->error.empty()) );

    std::string tip = (resolved != nullptr) ? resolved->full : name;

    if ( (resolved != nullptr) &&
         (resolved->has_value) )
    {
        tip += " = " + brief(*resolved, _model.doc.width);
    }

    if (broken)
    {
        tip += "\n" + resolved->error;
    }

    option_set made = attrs({ { "trailing_text", kind_tag(_row) },
                              { "open", group && !_row.collapsed },
                              { "leaf", !group },
                              { "selected", is_selected(_model, _row.id) },
                              { "action", "select" },
                              { "toggle_action", "toggle_open" },
                              { "drag_type", "bitwisedom.row" },
                              { "drop_action", "drop" },
                              { "context_action", "context.tree" },
                              { "double_action", "rename.tree" },
                              { "r", static_cast<double>(channel(colour,
                                                                 0)) },
                              { "g", static_cast<double>(channel(colour,
                                                                 1)) },
                              { "b", static_cast<double>(channel(colour,
                                                                 2)) },
                              { "a", 1.0 },
                              { "tooltip", tip + "\n(double-click or F2 to "
                                                 "rename; right-click for "
                                                 "more)" },
                              { "key", "row" + format_dec(
                                           static_cast<u64>(_row.id)) } });

    // renamed here: the node's text is an input. Only the row being renamed
    // carries the input's attributes -- the rest of a thousand rows are
    // built faster without them, and their absence is the same as false
    if ( (_model.ui.renaming == _row.id) &&
         (_model.ui.rename_where == "tree") )
    {
        made.set("editing", true);
        made.set("edit_text", _model.ui.rename_text);
        made.set("edit_action", std::string("rename.commit"));
        made.set("cancel_action", std::string("rename.cancel"));
        made.set("focus", _model.ui.rename_focus);
    }

    return tree_node((broken ? "! " : "") + name,
                     _row.id,
                     std::move(children),
                     std::move(made));
}

element_template
tree_panel(
    const model& _model,
    double       _height
)
{
    templates rows;

    for (const row& child : _model.doc.root.children)
    {
        rows.push_back(tree_row(_model, child));
    }

    if (rows.empty())
    {
        rows.push_back(muted("An empty sheet. Add a bit, a range or a "
                             "group with the buttons above, or open an "
                             "example from the File menu."));
    }

    const option_set small = attrs({ { "small", true } });
    const openness   groups = openness_of(_model.doc);

    return panel("left",
                 { label("Symbols", attrs({ { "tone", "heading" } })),
                   hrow({ action_button("+bit", "edit.add",
                                       attrs({ { "value", 0L },
                                               { "small", true },
                                               { "tooltip", "a one-bit "
                                                 "flag at the next free "
                                                 "bit" } })),
                         action_button("+range", "edit.add",
                                       attrs({ { "value", 1L },
                                               { "small", true } })),
                         action_button("+value", "edit.add",
                                       attrs({ { "value", 2L },
                                               { "small", true } })),
                         action_button("+group", "edit.add",
                                       attrs({ { "value", -1L },
                                               { "small", true } })) }),
                   hrow({ action_button("Expand all", "tree.open_all",
                                       attrs({ { "value", 0L },
                                               { "small", true },
                                               { "key", "open_all" },
                                               { "tooltip", "open every "
                                                 "group, in the tree and "
                                                 "the grid "
                                                 "(Ctrl+Shift+Plus)" } }),
                                       groups.closed),
                         action_button("Collapse all", "tree.close_all",
                                       attrs({ { "value", 0L },
                                               { "small", true },
                                               { "key", "close_all" },
                                               { "tooltip", "close every "
                                                 "group, in the tree and "
                                                 "the grid "
                                                 "(Ctrl+Shift+Minus)" } }),
                                       groups.open) }),
                   separator(),
                   panel("tree", std::move(rows),
                         attrs({ { "height", 0.0 } })) },
                 attrs({ { "width", static_cast<double>(_model.ui.left) },
                         { "height", _height } }));
}

// ------------------------------------------------------------ the inspector

templates
value_rows(
    const model&        _model,
    const resolved_row& _resolved
)
{
    const int width = _model.doc.width;

    if (!_resolved.has_value)
    {
        templates rows = { form_row("value",
                                    muted(_resolved.error.empty()
                                              ? std::string("none")
                                              : _resolved.error)) };

        if (!_resolved.expansion.empty())
        {
            rows.push_back(form_row("expands to",
                                    label(_resolved.expansion,
                                          attrs({ { "wrap", true } }))));
        }

        return rows;
    }

    templates rows;

    if (is_wide(width))
    {
        // a set of bits: how many, where, and in hex
        rows.push_back(form_row("bits set", label(format_dec(
                                    static_cast<u64>(
                                        _resolved.bits.count())))));
        rows.push_back(form_row("where", label(runs_text(_resolved.bits, 12),
                                               attrs({ { "wrap", true } }))));
        // past a thousand digits, hex says nothing a reader can take in
        const std::string digits = format_hex(_resolved.bits, 0, true);

        rows.push_back(form_row("hex",
                                label( (digits.size() <= 1024)
                                           ? "0x" + group_digits(digits, 2)
                                           : format_dec(static_cast<u64>(
                                                 digits.size())) +
                                                 " digits, too many to "
                                                 "show",
                                      attrs({ { "wrap", true } }))));

        const c_constant known = wide_constant(_resolved);

        rows.push_back(form_row("in C", label(known.ok
                                    ? (known.negative
                                           ? format_signed(
                                                 static_cast<i64>(
                                                     known.value))
                                           : format_dec(known.value))
                                    : known.error,
                                    attrs({ { "wrap", true } }))));
    }
    else
    {
        const u64 value = _resolved.value;

        rows.push_back(form_row("hex", label(hex(value, width))));
        rows.push_back(form_row("unsigned", label(format_dec(value))));
        rows.push_back(form_row("signed", label(format_signed(
                                    sign_extend(value, width)))));
        rows.push_back(form_row("binary",
                                label(format_bin(value, width, 8, ' '),
                                      attrs({ { "wrap", true } }))));
        rows.push_back(form_row("bits set", label(format_dec(
                                    static_cast<u64>(bit_count(value))))));
    }

    // a macro's value is C's: its type, and what its name expands to
    if (!_resolved.ctype.empty())
    {
        rows.push_back(form_row("C type", label(_resolved.ctype)));
    }

    if (!_resolved.expansion.empty())
    {
        rows.push_back(form_row("expands to",
                                label(_resolved.expansion,
                                      attrs({ { "wrap", true } }))));
    }

    return rows;
}

templates
definition_rows(
    const model&        _model,
    const row&          _row,
    const resolved_row& _resolved
)
{
    const long id    = _row.id;
    const int  top   = _model.doc.width - 1;
    templates  rows;

    // number
    //   an integer field over one of the row's fields.
    auto number = [id](long               _value,
                       const std::string& _field,
                       long               _low,
                       long               _high)
    {
        return int_field(_value, "row." + _field,
                         attrs({ { "target", id },
                                 { "min", _low },
                                 { "max", _high },
                                 { "width", 1.0 },
                                 { "key", _field } }));
    };

    switch (_row.def)
    {
        case def_kind::bit:
            rows.push_back(form_row("bit", number(_row.bit, "bit", 0, top)));
            break;

        case def_kind::range:
        {
            const int msb = _row.lsb + _row.width - 1;

            rows.push_back(form_row("lowest bit",
                                    number(_row.lsb, "lsb", 0, top)));
            rows.push_back(form_row("width",
                                    number(_row.width, "width", 1,
                                           _model.doc.width)));
            rows.push_back(form_row("highest bit",
                                    number(msb, "msb", _row.lsb, top)));
            rows.push_back(form_row(
                "from byte",
                number((_model.doc.bytes == byte_order::little)
                           ? _row.lsb / 8
                           : (top - msb) / 8,
                       "byte_low", 0, top / 8)));
            rows.push_back(form_row(
                "to byte",
                number((_model.doc.bytes == byte_order::little)
                           ? msb / 8
                           : (top - _row.lsb) / 8,
                       "byte_high", 0, top / 8)));
            break;
        }

        case def_kind::field_value:
        {
            const std::vector<int>   ranges = range_choices(_model.doc);
            std::vector<std::string> names  = { "(its own bits)" };
            long                     chosen = 0;

            for (std::size_t i = 0; i < ranges.size(); ++i)
            {
                const resolved_row* each = _model.res.find(ranges[i]);
                names.push_back((each != nullptr) ? each->full : "?");

                if (ranges[i] == _row.field)
                {
                    chosen = static_cast<long>(i) + 1;
                }
            }

            rows.push_back(form_row("field",
                                    combo(names, chosen, "row.field",
                                          attrs({ { "target", id },
                                                  { "width", 1.0 } }))));

            if (_row.field == 0)
            {
                rows.push_back(form_row("lowest bit",
                                        number(_row.lsb, "lsb", 0, top)));
                rows.push_back(form_row("width",
                                        number(_row.width, "width", 1,
                                               _model.doc.width)));
            }

            std::vector<std::string> encodings;

            for (int i = 0; i < ENCODING_COUNT; ++i)
            {
                encodings.emplace_back(
                    encoding_name(static_cast<encoding>(i)));
            }

            rows.push_back(form_row("encoding",
                                    combo(encodings,
                                          static_cast<long>(_row.enc),
                                          "row.encoding",
                                          attrs({ { "target", id },
                                                  { "width", 1.0 } }))));

            if (_row.enc == encoding::offset_binary)
            {
                rows.push_back(form_row(
                    "bias",
                    int_field(static_cast<long>(_row.bias), "row.bias",
                              attrs({ { "target", id },
                                      { "width", 1.0 },
                                      { "key", "bias" } }))));
            }

            rows.push_back(form_row(
                "number",
                text_field(format_signed(_row.number), "row.number",
                           attrs({ { "target", id },
                                   { "key", "number" },
                                   { "tooltip", "a signed number: -3, 0x1F, "
                                                "0b101" } }))));

            const int         width = std::max(1, _resolved.span_width);
            const value_range range = range_of(_row.enc, width, _row.bias);

            rows.push_back(form_row(
                "holds",
                muted(format_signed(range.low) + " .. " +
                      format_signed(range.high) + " in " +
                      format_dec(static_cast<u64>(width)) + " bits")));
            break;
        }

        case def_kind::literal:
            rows.push_back(form_row(
                "value",
                text_field(_row.negative
                               ? format_signed(sign_extend(_resolved.value,
                                                           _model.doc.width))
                               : "0x" + format_hex(_row.literal, 0, true),
                           "row.literal",
                           attrs({ { "target", id },
                                   { "key", "literal" },
                                   { "tooltip", "0x1F, 31, 037, 0b11111, "
                                                "or a negative number" } }))));
            rows.push_back(table_row({ label(""),
                                       toggle("written as negative",
                                              _row.negative,
                                              "row.negative",
                                              attrs({ { "target", id } })) }));
            rows.push_back(form_row(
                "least digits",
                int_field(_row.digits, "row.digits",
                          attrs({ { "target", id },
                                  { "min", 0L },
                                  { "max", 64L },
                                  { "width", 1.0 },
                                  { "key", "digits" },
                                  { "tooltip", "the fewest digits the "
                                               "header writes it with: "
                                               "0x0A00 is 4; 0 for as "
                                               "few as it needs" } }))));
            break;

        case def_kind::expression:
            rows.push_back(form_row(
                "C expression",
                text_field(_row.expr, "row.expr",
                           attrs({ { "target", id },
                                   { "key", "expr" },
                                   { "multiline", true },
                                   { "height", 76.0 },
                                   { "tooltip", "C's integer operators over "
                                                "other symbols' names, with "
                                                "BIT(n), MASK(w) and "
                                                "GENMASK(hi, lo). Supports "
                                                "multiline C-style backslash "
                                                "continuations." } }))));
            break;

        case def_kind::macro:
            rows.push_back(form_row("parameters",
                                    text_field(_row.params, "row.params",
                                               attrs({ { "target", id },
                                                       { "key", "params" },
                                                       { "placeholder",
                                                         "x, v" } }))));
            if (_row.params.empty())
            {
                rows.push_back(table_row({
                    label(""),
                    toggle("function-like: NAME()", _row.callable,
                           "row.callable",
                           attrs({ { "target", id },
                                   { "tooltip", "a macro that takes no "
                                                "arguments but is called "
                                                "with parentheses" } })) }));
            }

            rows.push_back(form_row("body",
                                    text_field(_row.body, "row.body",
                                               attrs({ { "target", id },
                                                       { "key", "body" },
                                                       { "multiline", true },
                                                       { "height", 90.0 },
                                                       { "tooltip",
                                                         "C, with {NAME} "
                                                         "for a symbol's "
                                                         "macro name. Its "
                                                         "lines are kept "
                                                         "as they are "
                                                         "written" } }))));
            break;
    }

    return rows;
}

templates
common_rows(
    const model& _model,
    const row&   _row
)
{
    const long          id       = _row.id;
    const resolved_row* resolved = _model.res.find(_row.id);
    const rgba          colour   = (resolved != nullptr) ? resolved->colour
                                                         : rgba(0);

    // text
    //   a text field over one of the row's fields.
    auto text = [id](const std::string& _value,
                     const std::string& _field,
                     const std::string& _tip)
    {
        return text_field(_value, "row." + _field,
                          attrs({ { "target", id },
                                  { "key", _field },
                                  { "tooltip", _tip } }));
    };

    templates rows;

    rows.push_back(form_row("short name",
                            text(_row.short_name, "short",
                                 "what the grid shows when the name will "
                                 "not fit")));
    rows.push_back(form_row("note",
                            text(_row.note, "note",
                                 "the brief in the generated header")));
    rows.push_back(form_row("prefix",
                            text(_row.prefix, "prefix",
                                 (_row.kind == row_kind::group)
                                     ? "put before every name in the group"
                                     : "put before this name")));
    rows.push_back(form_row("suffix",
                            text(_row.suffix, "suffix",
                                 (_row.kind == row_kind::group)
                                     ? "put after every name in the group"
                                     : "put after this name")));
    rows.push_back(form_row(
        "colour",
        hrow({ color_field(channel(colour, 0), channel(colour, 1),
                          channel(colour, 2), channel(colour, 3),
                          "row.colour",
                          attrs({ { "target", id } })),
              action_button("Auto", "row.auto_colour",
                            attrs({ { "target", id }, { "small", true } }),
                            _row.colour != 0 || _row.transparent),
              toggle("No fill", _row.transparent, "row.transparent",
                     attrs({ { "target", id },
                             { "tooltip", "Transparent cells: no row "
                                          "fill or range tint" } })) })));
    rows.push_back(form_row(
        "literals",
        combo({ "as the group", "hex", "decimal", "octal", "binary",
                "shifts" },
              static_cast<long>(_row.fmt) + 1, "row.radix",
              attrs({ { "target", id }, { "width", 1.0 } }))));
    rows.push_back(form_row(
        "suffix type",
        combo({ "as the group", "none", "u", "ul", "ull" },
              static_cast<long>(_row.suf) + 1, "row.int_suffix",
              attrs({ { "target", id }, { "width", 1.0 } }))));
    rows.push_back(table_row({ label(""),
                               toggle("in the header", _row.emit, "row.emit",
                                      attrs({ { "target", id } })) }));

    return rows;
}

element_template
issue_list(
    const model& _model,
    int          _id
)
{
    templates lines;

    for (const issue& each : _model.res.issues)
    {
        if (each.row != _id)
        {
            continue;
        }

        lines.push_back(level_label(each.message, each.level, true));
    }

    return column(std::move(lines), 1, attrs({ { "key", "issues" } }));
}

element_template
symbol_inspector(
    const model& _model,
    const row&   _row
)
{
    const resolved_row* resolved = _model.res.find(_row.id);

    if (resolved == nullptr)
    {
        return muted("resolving...");
    }

    templates top;

    top.push_back(form_row("name",
                           text_field(_row.name, "row.name",
                                      attrs({ { "target", static_cast<long>(
                                                              _row.id) },
                                              { "key", "name" },
                                              { "commit", true },
                                              { "tooltip",
                                                "Enter renames it, and "
                                                "every reference to it" } }))));
    top.push_back(form_row("kind",
                           combo(KIND_NAMES,
                                 static_cast<long>(_row.def),
                                 "row.def",
                                 attrs({ { "target", static_cast<long>(
                                                         _row.id) },
                                         { "width", 1.0 } }))));

    for (element_template& each : definition_rows(_model, _row, *resolved))
    {
        top.push_back(std::move(each));
    }

    if (_row.def == def_kind::range)
    {
        top.push_back(table_row({ label(""),
            toggle("Use name as prefix", _row.name_children,
                   "row.name_children",
                   attrs({ { "target", static_cast<long>(_row.id) },
                           { "tooltip", "Prepend this range name to "
                                        "nested symbol names" } })) }));
        top.push_back(table_row({ label(""),
            toggle("Collapse children", _row.collapsed,
                   "row.collapsed",
                   attrs({ { "target", static_cast<long>(_row.id) } })) }));
        top.push_back(table_row({ label(""),
            toggle("Exclusive children", _row.exclusive,
                   "row.exclusive",
                   attrs({ { "target", static_cast<long>(_row.id) },
                           { "tooltip", "Warn for conflicting sibling "
                                        "allocations; contained ranges "
                                        "are allowed" } })) }));
    }

    templates actions;
    if (_row.def == def_kind::range)
    {
        actions.push_back(hrow({
            action_button("+bit", "edit.add", attrs({ { "value", 0L } })),
            action_button("+range", "edit.add", attrs({ { "value", 1L } })),
            action_button("+expr", "edit.add", attrs({ { "value", 4L } }))
        }));
    }

    return column({
        coloured(resolved->full.empty() ? "(unnamed)" : resolved->full,
                 resolved->colour),
        issue_list(_model, _row.id),
        shared_form("definition", std::move(top)),
        section("Value", { shared_form("value",
                                       value_rows(_model, *resolved)),
                           hrow({ action_button("Into the probe", "probe.row",
                                               attrs({ { "value",
                                                         static_cast<long>(
                                                             _row.id) },
                                                       { "small", true } }),
                                               resolved->has_value) }) }),
        section("Naming and format", { shared_form("common",
                                                    common_rows(_model,
                                                                _row)) }),
        column(std::move(actions)),
        separator(),
        hrow({ action_button("Duplicate", "edit.duplicate"),
              action_button("Delete", "edit.delete"),
              action_button("Up", "edit.move_up"),
              action_button("Down", "edit.move_down") }) });
}

element_template
group_inspector(
    const model& _model,
    const row&   _row
)
{
    const resolved_row* resolved = _model.res.find(_row.id);
    const long          id       = _row.id;

    if (resolved == nullptr)
    {
        return muted("resolving...");
    }

    templates top;

    top.push_back(form_row("name",
                           text_field(_row.name, "row.name",
                                      attrs({ { "target", id },
                                              { "key", "name" },
                                              { "commit", true } }))));
    top.push_back(table_row({ label(""),
                              toggle("collapsed", _row.collapsed,
                                     "row.collapsed",
                                     attrs({ { "target", id } })) }));
    const std::string isolate_tip = "names here ignore the prefixes and "
                                    "suffixes of the groups around this one";

    top.push_back(table_row({ label(""),
                              toggle("own names only",
                                     _row.isolate, "row.isolate",
                                     attrs({ { "target", id },
                                             { "tooltip", isolate_tip } })) }));
    top.push_back(table_row({ label(""),
                              toggle("Use name as prefix",
                                     _row.name_children, "row.name_children",
                                     attrs({ { "target", id },
                                             { "tooltip",
                                               "Include this group's name "
                                               "before every descendant's "
                                               "name. Nested groups can "
                                               "also add their names." } })) }));
    top.push_back(table_row({ label(""),
                              toggle("exclusive bits",
                                     _row.exclusive, "row.exclusive",
                                     attrs({ { "target", id },
                                             { "tooltip",
                                               "warn when two of its bit or "
                                               "range symbols share a "
                                               "bit" } })) }));

    return column({
        coloured("group " + (_row.name.empty() ? std::string("(unnamed)")
                                               : _row.name),
                 resolved->colour),
        muted(format_dec(static_cast<u64>(resolved->children)) +
              (_row.name_children
                   ? " rows; group name prefixes descendants"
                   : " rows; group name is not inherited")),
        shared_form("group", std::move(top)),
        section("Union of its symbols", { shared_form("value",
                                                      value_rows(
                                                          _model,
                                                          *resolved)) }),
        section("Naming and format", { shared_form("common",
                                                   common_rows(_model,
                                                               _row)) }),
        separator(),
        hrow({ action_button("+bit", "edit.add", attrs({ { "value", 0L } })),
              action_button("+range", "edit.add", attrs({ { "value", 1L } })),
              action_button("+group", "edit.add",
                            attrs({ { "value", -1L } })),
              action_button("Delete", "edit.delete") }) });
}

element_template
sheet_inspector(
    const model& _model
)
{
    int symbols = 0;

    for (const resolved_row& each : _model.res.rows)
    {
        symbols += (each.kind == row_kind::symbol) ? 1 : 0;
    }

    return column({
        coloured(_model.doc.title, _model.shown.colours.accent),
        muted(format_dec(static_cast<u64>(symbols)) + " symbols in " +
              format_dec(static_cast<u64>(_model.doc.width)) + " bits; " +
              format_dec(static_cast<u64>(_model.res.allocated.count())) +
              " bits allocated, " +
              format_dec(static_cast<u64>(_model.res.collisions.count())) +
              " colliding."),
        muted("Select a row in the tree or the grid to inspect it. Click a "
              "cell to set or move a bit, drag across cells to define a "
              "range, and click a column heading to collapse its segment."),
        shared_form("sheet", setting_rows(_model, { "sheet.title",
                                                    "sheet.width",
                                                    "sheet.prefix",
                                                    "sheet.suffix",
                                                    "sheet.numbering",
                                                    "sheet.msb_left",
                                                    "sheet.bytes" })) });
}

element_template
inspector_panel(
    const model& _model,
    double       _height
)
{
    const row*       selected = inspected(_model);
    element_template content  =
        (selected == nullptr)                 ? sheet_inspector(_model)
      : (selected->kind == row_kind::group)   ? group_inspector(_model,
                                                                *selected)
                                              : symbol_inspector(_model,
                                                                 *selected);

    if (_model.ui.selection.size() > 1)
    {
        content = column({ muted(format_dec(_model.ui.selection.size()) +
                                 " rows selected; the first is shown, and "
                                 "operations act on all of them."),
                           std::move(content) });
    }

    return panel("right",
                 { std::move(content) },
                 attrs({ { "height", _height } }));
}

// ----------------------------------------------------------- the bottom tabs

element_template
operations_tab(
    const model& _model
)
{
    std::vector<std::string> names;

    for (int i = 0; i < OP_COUNT; ++i)
    {
        const op_info info = describe(static_cast<op_kind>(i));
        names.push_back(std::string(info.name) +
                        ((info.spelling[0] != '\0')
                             ? std::string("   ") + info.spelling
                             : std::string()));
    }

    const op_kind op      = static_cast<op_kind>(_model.ui.op);
    const op_info info    = describe(op);
    const bool    operand = (info.operand[0] != '\0');
    const auto    targets = op_targets(_model);

    templates rows;

    for (const op_row& each : _model.preview.rows)
    {
        const resolved_row* resolved = _model.res.find(each.id);
        const std::string   name     = (resolved != nullptr) ? resolved->full
                                                             : "?";

        rows.push_back(table_row({
            selectable(name, false, "select", each.id,
                       attrs({ { "span", true },
                               { "context_action", "context.table" },
                               { "double_action", "rename.table" } })),
            label(brief(each.before, _model.doc.width)),
            label(brief(each.after, _model.doc.width)),
            label(each.lost > 0 ? format_dec(static_cast<u64>(each.lost))
                                : std::string("-"),
                  attrs({ { "tone", (each.lost > 0) ? "error" : "muted" } })),
            label(each.gained > 0 ? format_dec(static_cast<u64>(each.gained))
                                  : std::string("-"),
                  attrs({ { "tone", "muted" } })),
            label(each.rewrite) }));
    }

    if (_model.preview.probe)
    {
        rows.push_back(table_row({
            label("(probe)", attrs({ { "tone", "accent" } })),
            label(brief(_model.preview.probe_before, _model.doc.width)),
            label(brief(_model.preview.probe_after, _model.doc.width)),
            label(""), label(""), label("the probe value") }));
    }

    const int before = _model.preview.collisions_before.count();
    const int after  = _model.preview.collisions_after.count();

    return column({
        hrow({ combo(names, _model.ui.op, "ops.op",
                    attrs({ { "width", 220.0 } })),
              text_field(_model.ui.operand, "ops.operand",
                         attrs({ { "width", 110.0 },
                                 { "key", "operand" },
                                 { "enabled", operand },
                                 { "placeholder", info.operand } })),
              toggle("the probe too", _model.ui.op_probe, "ops.probe"),
              toggle("show in the grid", _model.ui.op_preview,
                     "ops.preview"),
              action_button("Apply", "ops.apply",
                            attrs({ { "tone", "accent" } }),
                            ( (!targets.empty()) ||
                              (_model.ui.op_probe) )) }),
        muted(targets.empty()
                  ? std::string("Select rows to operate on -- a group stands "
                                "for every symbol in it -- or include the "
                                "probe.")
                  : format_dec(_model.preview.rows.size()) +
                        " symbol(s). Collisions: " +
                        format_dec(static_cast<u64>(before)) + " bit(s) " +
                        "before, " + format_dec(static_cast<u64>(after)) +
                        " after."),
        flow_view(attrs({ { "height", 128.0 } })),
        table("preview",
              { "symbol", "before", "after", "lost", "gained", "becomes" },
              std::move(rows),
              attrs({ { "borders", "rows" },
                      { "key", "table.preview" },
                      { "widths", "0\n110\n110\n44\n52\n0" } })) });
}

element_template
macros_tab(
    const model& _model
)
{
    templates notes;

    for (const std::string& note : _model.header.notes)
    {
        notes.push_back(label(note, attrs({ { "tone", "accent" },
                                            { "wrap", true } })));
    }

    // narrow
    //   a setting's control at a fixed width, for a row of them.
    auto narrow = [&_model](const char* _path,
                            double      _width)
    {
        return setting_control(_model, _path, attrs({ { "width", _width } }));
    };

    templates second = { setting_control(_model, "gen.banner"),
                         setting_control(_model, "gen.guard"),
                         setting_control(_model, "gen.pragma_once"),
                         setting_control(_model, "gen.comments"),
                         setting_control(_model, "gen.sections"),
                         setting_control(_model, "gen.align"),
                         setting_control(_model, "gen.pad"),
                         setting_control(_model, "gen.upper_hex"),
                         label("data model"),
                         narrow("gen.model", 170.0) };

    // a wide sheet is written as arrays of words
    if (is_wide(_model.doc.width))
    {
        second.push_back(label("array words"));
        second.push_back(narrow("gen.word", 90.0));
    }

    return column({
        hrow({ narrow("gen.level", 90.0),
              narrow("gen.form", 130.0),
              narrow("gen.style", 100.0),
              narrow("gen.base", 90.0),
              narrow("gen.suffix", 64.0),
              label("file"),
              narrow("gen.file_name", 160.0),
              action_button("Copy", "gen.copy"),
              action_button("Export", "file.export",
                            attrs({ { "tone", "accent" } })) }),
        hrow(std::move(second)),
        section("Environment: #defines the macros are evaluated in",
                { muted("Definitions from elsewhere -- another header, a "
                        "compiler's options -- that the sheet's macros "
                        "use and the header does not write. Conditionals "
                        "are followed; <stdint.h>, <limits.h> and "
                        "<stdbool.h> are already here."),
                  text_field(_model.doc.environment, "set",
                             attrs({ { "target", "sheet.environment" },
                                     { "key", "environment" },
                                     { "multiline", true },
                                     { "commit", true },
                                     { "height", 90.0 } })) }),
        column(std::move(notes), 1, attrs({ { "key", "notes" } })),
        code_view(attrs({ { "height", 0.0 } })) });
}

element_template
templates_tab(
    const model& _model
)
{
    const char* const kinds[] = { "bit", "range", "value", "literal",
                                  "expr" };
    const bool        wide    = is_wide(_model.doc.width);
    templates         rows;

    for (std::size_t i = 0; i < _model.doc.gen.templates.size(); ++i)
    {
        const macro_template& made   = _model.doc.gen.templates[i];
        const std::string     prefix = format_dec(i) + ".";

        // a sheet's own kind: a wide sheet's templates, or a narrow one's
        if (made.wide != wide)
        {
            continue;
        }

        // field
        //   one editable column of a template.
        auto field = [&prefix](const std::string& _value,
                               const std::string& _name)
        {
            return text_field(_value, "template.set",
                              attrs({ { "target", prefix + _name },
                                      { "key", prefix + _name } }));
        };

        templates applies;

        for (int k = 0; k < 5; ++k)
        {
            applies.push_back(toggle(kinds[k],
                                     (made.applies & (1 << k)) != 0,
                                     "template.set",
                                     attrs({ { "target", prefix + "applies." +
                                                         format_dec(
                                                             static_cast<u64>(
                                                                 k)) } })));
        }

        rows.push_back(table_row({
            toggle("", made.enabled, "template.set",
                   attrs({ { "target", prefix + "enabled" } })),
            field(made.name, "name"),
            field(made.params, "params"),
            field(made.body, "body"),
            hrow(std::move(applies), 2),
            field(made.note, "note"),
            action_button("x", "template.remove",
                          attrs({ { "value", static_cast<long>(i) },
                                  { "small", true } })) }));
    }

    const std::string help =
        wide ? "Macros written beside every symbol of the kinds ticked. This "
               "sheet is wider than an integer, so its symbols are places "
               "in arrays of words, and these are its own templates; a "
               "narrow sheet keeps others. In the name, parameters and body: "
               "{NAME} is the symbol's macro name, {LOCAL} its own, {VALUE} "
               "its value, {WIDTH} its width, {BIT} its bit, {PREFIX} and "
               "{SUFFIX} its affixes; {WORD} is the word a bit or range "
               "begins in, {WORD_SHIFT} where in it, {WORD_MASK} its bits "
               "there and {WORD_BITS} a word's width; {FIELD} is a value's "
               "range; {GET} reads a range from an array named a, and {PUT} "
               "writes v into it; {BITSET} is the prefix of the array "
               "macros. No parameters writes an object-like macro."
             : "Macros written beside every symbol of the kinds ticked. In "
               "the name, parameters and body: {NAME} is the symbol's macro "
               "name, {LOCAL} its own, {VALUE} its value, {MASK} its mask (a "
               "value's field), {SHIFT} its lowest bit, {WIDTH} its width, "
               "{BIT} its bit, {PREFIX} and {SUFFIX} its affixes. No "
               "parameters writes an object-like macro.";

    return column({
        muted(help),
        hrow({ action_button("Add a template", "template.add"),
              action_button("Restore the defaults", "template.defaults") }),
        table("templates",
              { "on", "name", "parameters", "body", "applies to", "note",
                "" },
              std::move(rows),
              attrs({ { "borders", "inner" },
                      { "key", "table.templates" },
                      { "widths", "28\n150\n70\n0\n300\n160\n24" } })) });
}

element_template
issues_tab(
    const model& _model
)
{
    templates rows;

    for (const issue& each : _model.res.issues)
    {
        const resolved_row* where = _model.res.find(each.row);
        const char*         level =
            (each.level == severity::error)   ? "error"
          : (each.level == severity::warning) ? "warning"
                                              : "note";

        // a sheet-wide issue is the environment's
        if (where == nullptr)
        {
            rows.push_back(table_row({
                level_label(level, each.level, false),
                label("(environment)", attrs({ { "tone", "muted" } })),
                label(each.message, attrs({ { "wrap", true } })) }));

            continue;
        }

        rows.push_back(table_row({
            level_label(level, each.level, false),
            selectable(where->full,
                       is_selected(_model, each.row),
                       "select",
                       each.row,
                       attrs({ { "span", true },
                               { "context_action", "context.table" },
                               { "double_action", "rename.table" } })),
            label(each.message, attrs({ { "wrap", true } })) }));
    }

    templates page;

    // what the last import said
    if (!_model.ui.import_notes.empty())
    {
        templates notes;

        for (const std::string& each : _model.ui.import_notes)
        {
            notes.push_back(muted(each));
        }

        page.push_back(section("From the import of " + _model.ui.import_name +
                                   " (" + format_dec(
                                       _model.ui.import_notes.size()) + ")",
                               std::move(notes)));
    }

    if (rows.empty())
    {
        page.push_back(label("No issues: every symbol resolves, nothing "
                             "collides, and every name is a C name.",
                             attrs({ { "tone", "ok" } })));

        return column(std::move(page));
    }

    page.push_back(table("issues", { "level", "symbol", "message" },
                         std::move(rows),
                         attrs({ { "borders", "rows" },
                                 { "key", "table.issues" },
                                 { "widths", "70\n220\n0" } })));

    return column(std::move(page));
}

element_template
grid_tab(
    const model& _model
)
{
    struct line_row
    {
        const char* key;
        const char* name;
    };

    const line_row lines[] = { { "minor", "between bits" },
                               { "major", "every n bits" },
                               { "rows", "between rows" },
                               { "groups", "above groups" },
                               { "segments", "segment edges" },
                               { "border", "around the grid" } };
    templates      rows;

    for (const line_row& each : lines)
    {
        const std::string base = std::string("grid.") + each.key;

        rows.push_back(table_row({
            label(each.name),
            setting_control(_model, base + ".show",
                            attrs({ { "text", "" } })),
            setting_control(_model, base + ".thickness"),
            setting_control(_model, base + ".colour"),
            setting_control(_model, base + ".style") }));
    }

    return hrow({
        panel("lines",
              { hrow({ setting_control(_model, "grid.lines"),
                       label("major line every"),
                       setting_control(_model, "grid.major_every",
                                       attrs({ { "width", 60.0 } })),
                       label("bits") }),
                table("lines",
                      { "line", "show", "thickness", "colour", "style" },
                      std::move(rows),
                      attrs({ { "borders", "rows" },
                              { "key", "table.lines" },
                              { "widths", "110\n44\n0\n52\n90" } })) },
              attrs({ { "width", 560.0 }, { "height", 0.0 } })),
        panel("cells",
              { form("cells", setting_rows(_model, {
                         "grid.fill",
                         "grid.cell_width", "grid.cell_height",
                         "grid.cell_gap", "grid.rounding",
                         "grid.name_width", "grid.value_width",
                         "grid.indent", "grid.index_every" }), 120.0) },
              attrs({ { "width", 300.0 }, { "height", 0.0 } })),
        panel("labels",
              { form("labels", setting_rows(_model, {
                         "grid.show_indices", "grid.show_digits",
                         "grid.show_labels", "grid.show_cell_boxes",
                         "grid.show_probe", "grid.show_usage",
                         "grid.full_names" }), 8.0) },
              attrs({ { "width", 0.0 }, { "height", 0.0 } })) });
}

element_template
colours_tab(
    const model& _model
)
{
    return hrow({
        panel("scheme",
              { muted(std::string("The theme and the colour scheme are the "
                                  "window's: Theme menu, now ") +
                      theme_name(_model.ui.theme) + ", " +
                      scheme_name(_model.ui.scheme) + ". What is set here "
                      "is the sheet's own, and stays with it."),
                form("scheme", setting_rows(_model, {
                         "colours.palette",
                         "colours.saturation", "colours.value",
                         "colours.mode", "colours.set_alpha",
                         "colours.range_alpha" }), 140.0),
                hrow({ action_button("Use the scheme's colours",
                                     "colours.automatic",
                                     attrs({ { "tooltip",
                                               "every colour here, and the "
                                               "grid lines', back to the "
                                               "theme's and scheme's; the "
                                               "rows' own colours stay" } }))
                     }) },
              attrs({ { "width", 400.0 }, { "height", 0.0 } })),
        panel("roles",
              { form("roles", setting_rows(_model, {
                         "colours.accent", "colours.empty_cell",
                         "colours.collision", "colours.probe",
                         "colours.selection", "colours.hover",
                         "colours.ghost" }), 120.0) },
              attrs({ { "width", 0.0 }, { "height", 0.0 } })) });
}

element_template
columns_tab(
    const model& _model
)
{
    templates rows;

    for (std::size_t i = 0; i < _model.doc.segments.size(); ++i)
    {
        const column_segment& each   = _model.doc.segments[i];
        const std::string     prefix = format_dec(i) + ".";
        const rgba            colour = (each.colour != 0)
                                           ? each.colour
                                           : _model.shown.colours.accent;

        rows.push_back(table_row({
            text_field(each.name, "segment.set",
                       attrs({ { "target", prefix + "name" },
                               { "key", prefix + "name" } })),
            int_field(each.lsb, "segment.set",
                      attrs({ { "target", prefix + "lsb" },
                              { "min", 0L },
                              { "max", static_cast<long>(_model.doc.width -
                                                         1) },
                              { "drag", true },
                              { "width", 1.0 } })),
            int_field(each.msb, "segment.set",
                      attrs({ { "target", prefix + "msb" },
                              { "min", 0L },
                              { "max", static_cast<long>(_model.doc.width -
                                                         1) },
                              { "drag", true },
                              { "width", 1.0 } })),
            color_field(channel(colour, 0), channel(colour, 1),
                        channel(colour, 2), channel(colour, 3),
                        "segment.set",
                        attrs({ { "target", prefix + "colour" } })),
            toggle("", each.collapsed, "segment.set",
                   attrs({ { "target", prefix + "collapsed" } })),
            action_button("x", "segment.remove",
                          attrs({ { "value", static_cast<long>(i) },
                                  { "small", true } })) }));
    }

    return hrow({
        panel("segments",
              { muted("Segments name runs of columns. A collapsed segment "
                      "is one narrow column; click its heading in the grid "
                      "to collapse or expand it."),
                hrow({ action_button("Add a segment", "segment.add",
                                    attrs({ { "tooltip", "over the selected "
                                              "range, or bits 0..7" } })) }),
                table("segments",
                      { "name", "lowest", "highest", "colour", "collapsed",
                        "" },
                      std::move(rows),
                      attrs({ { "borders", "rows" },
                              { "key", "table.segments" },
                              { "widths", "0\n70\n70\n52\n70\n24" } })) },
              attrs({ { "width", 560.0 }, { "height", 0.0 } })),
        panel("abbreviation",
              { form("abbreviation", setting_rows(_model, {
                         "grid.auto_segments", "grid.collapsed",
                         "grid.abbreviate_unused", "grid.unused_run" }),
                     170.0) },
              attrs({ { "width", 0.0 }, { "height", 0.0 } })) });
}

element_template
sheet_tab(
    const model& _model
)
{
    return hrow({
        panel("sheet",
              { form("sheet", setting_rows(_model, {
                         "sheet.title", "sheet.width", "sheet.prefix",
                         "sheet.suffix", "sheet.numbering",
                         "sheet.msb_left", "sheet.bytes" }), 150.0) },
              attrs({ { "width", 460.0 }, { "height", 0.0 } })),
        panel("probe",
              { label("The probe", attrs({ { "tone", "heading" } })),
                muted("A scratch value, shown in the grid's top row. Click "
                      "its cells to flip bits, or type a value. The "
                      "operations act on it when asked."),
                hrow({ text_field("0x" + format_hex(_model.doc.probe, 0,
                                                    true),
                                 "probe.text",
                                 attrs({ { "width", 200.0 },
                                         { "key", "probe" } })),
                      action_button("Clear", "probe.clear") }),
                label(is_wide(_model.doc.width)
                          ? runs_text(_model.doc.probe, 12)
                          : format_bin(_model.doc.probe.low(),
                                       _model.doc.width, 8, ' '),
                      attrs({ { "wrap", true } })) },
              attrs({ { "width", 0.0 }, { "height", 0.0 } })) });
}

// naming_tab
//   Sheet-wide boundary trimming, kept out of the per-symbol inspector.
element_template
naming_tab(const model& _model)
{
    templates preview;
    const row* selected = find_row(_model.doc, _model.ui.selection.empty()
        ? 0 : _model.ui.selection.front());
    const resolved_row* resolved = selected != nullptr
        ? _model.res.find(selected->id) : nullptr;

    if (resolved != nullptr)
    {
        preview.push_back(label("Selected symbol",
                                attrs({ { "tone", "heading" } })));
        preview.push_back(form("naming_preview", {
            form_row("local", label(selected->name)),
            form_row("full", label(resolved->full))
        }, 90.0));
    }
    else
    {
        preview.push_back(muted("Select a symbol or group to see its "
                                "effective name here."));
    }

    return hrow({
        panel("trim_rules",
              { label("Trim local names",
                      attrs({ { "tone", "heading" } })),
                muted("Comma-separated exact characters or sequences "
                      "to remove from the ends of each node's name. "
                      "Empty fields keep the name unchanged."),
                form("trimming", setting_rows(_model, {
                    "sheet.trim_both", "sheet.trim_left",
                    "sheet.trim_right" }), 125.0),
                muted("Example: Right only = _ removes trailing _ "
                      "from D_FOO_ before names are joined. "
                      "Existing prefixes and suffixes stay untouched.") },
              attrs({ { "width", 440.0 }, { "height", 0.0 } })),
        panel("name_preview", std::move(preview),
              attrs({ { "width", 0.0 }, { "height", 0.0 } }))
    });
}

// page
//   one page of the tabbed panel, by its place among the tabs.
element_template
page(
    const model& _model,
    int          _index
)
{
    switch (_index)
    {
        case 0:  return operations_tab(_model);
        case 1:  return macros_tab(_model);
        case 2:  return templates_tab(_model);
        case 3:  return issues_tab(_model);
        case 4:  return grid_tab(_model);
        case 5:  return colours_tab(_model);
        case 6:  return columns_tab(_model);
        case 7:  return naming_tab(_model);
        default: return sheet_tab(_model);
    }
}

// page_titles
//   the tabs' titles; the issues' says how many there are.
std::vector<std::string>
page_titles(
    const model& _model
)
{
    const int         errors   = _model.res.errors;
    const int         warnings = _model.res.warnings;
    const std::string issues   =
        "Issues" + ((errors + warnings > 0)
                        ? " (" + format_dec(static_cast<u64>(errors +
                                                             warnings)) + ")"
                        : std::string());

    return { "Operations", "Macros", "Templates", issues, "Grid lines",
             "Colours", "Columns", "Naming", "Sheet" };
}

// panel_buttons
//   the buttons at the end of the panel's tabs: the other ways it can be
// shown, from the way it is shown now.
templates
panel_buttons(
    const ui_state& _ui
)
{
    // way
    //   a button that shows the panel another way.
    auto way = [](const char* _icon,
                  panel_mode  _mode,
                  const char* _tip,
                  bool        _on = false)
    {
        return tab_button("", "panel.mode",
                          attrs({ { "icon", _icon },
                                  { "value", static_cast<long>(_mode) },
                                  { "tooltip", _tip },
                                  { "on", _on } }));
    };

    const element_template hide =
        way("close", panel_mode::hidden,
            "Hide it -- Ctrl+J, or View > Bottom panel, brings it back");

    switch (_ui.panel)
    {
        case panel_mode::minimized:
            return { way("restore", panel_mode::docked,
                         "Restore it below the grid"),
                     way("pin", panel_mode::peek,
                         "Open it over the grid while the mouse is on its "
                         "tabs"),
                     way("float", panel_mode::floating,
                         "Float it in a window of its own"),
                     hide };

        case panel_mode::peek:
            return { way("pin", panel_mode::docked,
                         "Keep it open, docked below the grid", true),
                     way("minimize", panel_mode::minimized,
                         "Minimize it to its tabs"),
                     way("float", panel_mode::floating,
                         "Float it in a window of its own"),
                     hide };

        case panel_mode::floating:
            return { way("dock", panel_mode::docked,
                         "Dock it below the grid"),
                     way("minimize", panel_mode::minimized,
                         "Minimize it to its tabs below the grid") };

        default:
            return { way("pin", panel_mode::peek,
                         "Minimize it until the mouse is over its tabs, "
                         "and then open it over the grid"),
                     way("minimize", panel_mode::minimized,
                         "Minimize it to its tabs"),
                     way("float", panel_mode::floating,
                         "Float it in a window of its own"),
                     hide };
    }
}

/*
panel_tabs
  The panel's tab bar: its pages, the front one with its content when the
panel shows content, and its buttons. Only the front page is built. On a
strip of tabs -- a panel minimized -- a press on any tab shows that page.
*/
element_template
panel_tabs(
    const model& _model,
    bool         _content
)
{
    const std::vector<std::string> titles = page_titles(_model);
    templates                      pages;

    for (std::size_t i = 0; i < titles.size(); ++i)
    {
        const int  index = static_cast<int>(i);
        const bool front = (index == _model.ui.tab);
        templates  content;

        if ( (_content) &&
             (front) )
        {
            content.push_back(page(_model, index));
        }

        option_set extra = attrs({ { "action", "tab" },
                                   { "value", static_cast<long>(i) },
                                   { "active", front } });

        if (!_content)
        {
            extra.set("click_action", "panel.open");
            extra.set("tooltip", "show the " + titles[i] + " page");
        }

        pages.push_back(tab(titles[i] + "###tab" + format_dec(i),
                            std::move(content),
                            ( (front) &&
                              (_model.ui.tab_forced) ),
                            extra));
    }

    for (element_template& each : panel_buttons(_model.ui))
    {
        pages.push_back(std::move(each));
    }

    return tabs("bottom_tabs", std::move(pages));
}

// strip_height
//   the height of a panel minimized to its tabs: a tab, and the panel's
// padding above and below it.
double
strip_height()
{
    return static_cast<double>(ImGui::GetFrameHeight() +
                               ImGui::GetStyle().WindowPadding.y * 2.0f) +
           2.0;
}

/*
bottom_area
  The tabbed panel as the model says it is shown, below the panels it
shares the window with: docked, under its splitter; or as a strip of its
tabs, which -- when it peeks -- opens the front page over the grid while
the mouse is on the strip or the page. A floating panel, and a hidden one,
take no room here.
*/
templates
bottom_area(
    const model& _model
)
{
    const ui_state& ui = _model.ui;
    templates       out;

    switch (ui.panel)
    {
        case panel_mode::docked:
            out.push_back(splitter(false, "layout.bottom",
                                   attrs({ { "thickness", SPLIT },
                                           { "style", "line" },
                                           { "key", "split.bottom" } })));
            out.push_back(panel("bottom",
                                { panel_tabs(_model, true) },
                                attrs({ { "height", 0.0 },
                                        { "key", "bottom" } })));
            break;

        case panel_mode::minimized:
        case panel_mode::peek:
        {
            const bool peek = (ui.panel == panel_mode::peek);

            // the page over the grid, its lower edge on the strip
            if ( (peek) &&
                 (ui.peeking) )
            {
                out.push_back(window(
                    "peek",
                    { page(_model, ui.tab) },
                    attrs({ { "mode", "overlay" },
                            { "width", 0.0 },
                            { "height", static_cast<double>(ui.bottom) },
                            { "keep", strip_height() },
                            { "linger", 0.45 },
                            { "leave_action", "panel.leave" },
                            { "key", "peek" } })));
            }

            // a hairline over the strip, as over a docked panel
            out.push_back(separator(std::string(),
                                    attrs({ { "key", "strip.line" } })));
            out.push_back(panel("strip",
                                { panel_tabs(_model, false) },
                                attrs({ { "height", strip_height() },
                                        { "hover_action",
                                          ( (peek) &&
                                            (!ui.peeking) ) ? "panel.peek"
                                                            : "" },
                                        { "hover_delay", 0.3 },
                                        { "key", "strip" } })));
            break;
        }

        default:
            break;
    }

    return out;
}

// ------------------------------------------------------- the context menu

templates
add_items()
{
    return {
        menu_item("Bit", "edit.add", attrs({ { "value", 0L } })),
        menu_item("Range", "edit.add", attrs({ { "value", 1L } })),
        menu_item("Value in a range", "edit.add", attrs({ { "value", 2L } })),
        menu_item("Literal", "edit.add", attrs({ { "value", 3L } })),
        menu_item("Expression", "edit.add", attrs({ { "value", 4L } })),
        menu_item("Macro", "edit.add", attrs({ { "value", 5L } })),
        separator(),
        menu_item("Group", "edit.add", attrs({ { "value", -1L } })) };
}

/*
context_menu
  The one popup the window has, its items made from what the model says it
was opened for: a row, a segment of the grid's columns, or the sheet.
*/
element_template
context_menu(
    const model& _model
)
{
    const ui_state& ui    = _model.ui;
    templates       items;

    if (ui.menu_kind == "row")
    {
        const row*          r        = find_row(_model.doc, ui.menu_row);
        const resolved_row* resolved = _model.res.find(ui.menu_row);
        const long          id       = ui.menu_row;
        bool                issues   = false;

        for (const issue& each : _model.res.issues)
        {
            issues = ( (issues) || (each.row == ui.menu_row) );
        }

        if (r != nullptr)
        {
            items.push_back(label((resolved != nullptr) ? resolved->full
                                                        : r->name,
                                  attrs({ { "tone", "muted" } })));
            items.push_back(separator());
            items.push_back(menu_item(
                "Rename", (ui.menu_where == "grid") ? "rename.grid"
                                                    : "rename.tree",
                attrs({ { "value", id }, { "shortcut", "F2" } })));
            items.push_back(menu_item("Copy as code", "code.copy",
                                      attrs({ { "value", id },
                                              { "shortcut",
                                                "Ctrl+Shift+C" } })));
            items.push_back(menu_item(
                can_contain(*r) ? "Paste code into it"
                                             : "Paste code after it",
                "code.paste",
                attrs({ { "value", id }, { "shortcut", "Ctrl+Shift+V" } })));
            items.push_back(separator());
            items.push_back(menu(can_contain(*r)
                                     ? "Add into it"
                                     : "Add after it",
                                 add_items()));
            items.push_back(menu_item("Duplicate", "edit.duplicate",
                                      attrs({ { "shortcut", "Ctrl+D" } })));
            items.push_back(menu_item("Delete", "edit.delete",
                                      attrs({ { "shortcut", "Del" } })));
            items.push_back(menu_item("Move up", "edit.move_up",
                                      attrs({ { "shortcut", "Alt+Up" } })));
            items.push_back(menu_item("Move down", "edit.move_down",
                                      attrs({ { "shortcut", "Alt+Down" } })));

            // a group, opened or closed with every group in it
            if (can_contain(*r))
            {
                items.push_back(separator());
                items.push_back(menu_item("Expand all inside",
                                          "tree.open_all",
                                          attrs({ { "value", id } })));
                items.push_back(menu_item("Collapse all inside",
                                          "tree.close_all",
                                          attrs({ { "value", id } })));
            }

            if ( (resolved != nullptr) &&
                 (resolved->has_value) )
            {
                items.push_back(separator());
                items.push_back(menu_item("Into the probe", "probe.row",
                                          attrs({ { "value", id } })));
            }

            if (issues)
            {
                items.push_back(menu_item("Show its issues", "tab.show",
                                          attrs({ { "value", 3L } })));
            }
        }
    }
    else if (ui.menu_kind == "segment")
    {
        const bool user      = ( (ui.menu_segment >= 0) &&
                                 (static_cast<std::size_t>(ui.menu_segment) <
                                  _model.doc.segments.size()) );
        const bool collapsed =
            ( (user) &&
              (_model.doc.segments[static_cast<std::size_t>(
                   ui.menu_segment)].collapsed) );
        const long index     = user ? static_cast<long>(ui.menu_segment)
                                    : -1L;
        const option_set band =
            attrs({ { "value", index },
                    { "lsb", static_cast<long>(ui.menu_lsb) },
                    { "msb", static_cast<long>(ui.menu_msb) } });

        items.push_back(label("bits " +
                                  format_dec(static_cast<u64>(ui.menu_msb)) +
                                  ".." +
                                  format_dec(static_cast<u64>(ui.menu_lsb)),
                              attrs({ { "tone", "muted" } })));
        items.push_back(separator());
        items.push_back(menu_item("Rename", "rename.segment", band));
        items.push_back(menu_item(collapsed ? "Expand" : "Collapse",
                                  "grid.segment", band));

        if (user)
        {
            items.push_back(menu_item("Remove the segment", "segment.remove",
                                      attrs({ { "value", index } })));
        }
    }
    else
    {
        items.push_back(menu("Add", add_items()));
        items.push_back(menu_item("Paste code", "code.paste",
                                  attrs({ { "value", 0L },
                                          { "shortcut", "Ctrl+Shift+V" } })));
        items.push_back(menu_item("Copy the whole sheet as code", "code.copy",
                                  attrs({ { "value", 0L } })));
        items.push_back(separator());
        items.push_back(menu_item("Expand all groups", "tree.open_all",
                                  attrs({ { "value", 0L },
                                          { "shortcut",
                                            "Ctrl+Shift+Plus" } })));
        items.push_back(menu_item("Collapse all groups", "tree.close_all",
                                  attrs({ { "value", 0L },
                                          { "shortcut",
                                            "Ctrl+Shift+Minus" } })));
    }

    return popup("context",
                 std::move(items),
                 attrs({ { "open", ui.menu_open },
                         { "key", "context" } }));
}

}  // anonymous namespace


element_template
build_ui(
    const model& _model
)
{
    const ui_state& ui = _model.ui;

    // the room the bottom panel takes below the others
    double below = 0.0;

    if (ui.panel == panel_mode::docked)
    {
        below = static_cast<double>(ui.bottom) + SPLIT;
    }
    else if ( (ui.panel == panel_mode::minimized) ||
              (ui.panel == panel_mode::peek) )
    {
        below = strip_height() + 1.0;
    }

    const double top    = -below;
    const double centre = ui.show_right
                              ? -(static_cast<double>(ui.right) + SPLIT)
                              : 0.0;

    // the panels, flush, divided by the hairlines that are the splitters
    templates middle;

    if (ui.show_left)
    {
        middle.push_back(tree_panel(_model, top));
        middle.push_back(splitter(true, "layout.left",
                                  attrs({ { "length", top },
                                          { "thickness", SPLIT },
                                          { "style", "line" } })));
    }

    middle.push_back(panel("centre",
                           { bit_grid() },
                           attrs({ { "width", centre },
                                   { "height", top } })));

    if (ui.show_right)
    {
        middle.push_back(splitter(true, "layout.right",
                                  attrs({ { "length", top },
                                          { "thickness", SPLIT },
                                          { "style", "line" } })));
        middle.push_back(inspector_panel(_model, top));
    }

    templates body;

    body.push_back(hrow(std::move(middle), 0, attrs({ { "key", "middle" } })));

    for (element_template& each : bottom_area(_model))
    {
        body.push_back(std::move(each));
    }

    templates whole = { menus(_model),
                        toolbar(_model),
                        column(std::move(body), 0) };

    // a floating panel, in a window of its own
    if (ui.panel == panel_mode::floating)
    {
        whole.push_back(window(
            "panel",
            { panel_tabs(_model, true) },
            attrs({ { "title", "Panel" },
                    { "x", 80.0 },
                    { "y", -40.0 },
                    { "width", 960.0 * static_cast<double>(ui_scale()) },
                    { "height", static_cast<double>(ui.bottom) + 40.0 },
                    { "close_action", "panel.close" } })));
    }

    whole.push_back(context_menu(_model));

    return column(std::move(whole));
}

}  // namespace gui
}  // namespace bitwisedom
