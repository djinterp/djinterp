/*******************************************************************************
* bitwisedom [app]                                                      view.cpp
*
* The interface, as uxoxo templates built from the window's state.
*   build_ui() is a pure function of what it is given -- the window around
* the documents, the document in front, and the documents of the workspace
* in front: it reads, never writes. The window is a menu bar over the
* workspace's dock space. The menu bar has File, Edit, View, Window and
* Tools -- the theme and colour scheme a submenu of View -- then the list of
* workspaces, the sheet file's name and the status line, and Help at its
* right end.
*   In the dock space: each document in a window of its own, its grid flush
* in it, and beside it a window for each definition file it imported; then
* each tool window the workspace has open -- the inspector, the macros and
* the templates, the operations, the issues, the settings pages, the
* display rules, and the symbols tree when the user wants it apart from the
* grid's symbols column. A tool window shows the document in front. The
* Options -- every setting, the window's own and the sheet's, and the
* settings files -- are a window that floats over the rest.
*   One control adds a row, as the user prefers it: a button and the menu
* of what to add, or a split button. The context menu is one popup, built
* from what the model says it is about: a row, a segment's band, the
* symbols column's heading or its buttons, or the sheet -- the sort and the
* display rules among them. The tree shows each group's rows in the order
* the grid does: the sheet's own, or sorted.
*   Every input that sits below something that can appear or vanish carries
* a key, so an edit in progress keeps its focus when, say, an error line
* appears above it. Every table carries one too: ImGui remembers a table's
* column widths by its identity, between frames and between runs.
*   A row is renamed in place: its tree node becomes an input when the
* model says it is being renamed there, and the grid draws its own. A
* closed group's rows are not built; Expand all and Collapse all, over the
* tree's window, on the View menu and on a group's menu, open and close
* every group at once.
*
* path:      /src/djinterp/tools/bitwisedom/app/view.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./view.hpp"  // corresponding header

// std
#include <algorithm>  // std::find, std::max, std::stable_sort
#include <cctype>     // std::tolower
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
#include <imgui.h>  // the frame's metrics, for the strip's height; colours
// bitwisedom
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_list
#include "./app.hpp"                       // range_choices, op_targets
#include "./attrs.hpp"                     // attrs
#include "./elements.hpp"                  // bit_grid, flow_view, ...
#include "./settings.hpp"                  // settings
#include "./shell.hpp"                     // shell_state, tool
#include "./theme.hpp"                     // ui_scale, legible, to_u32


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

// MENU_INDENT
//   constant: the room between the menu bar's last menu on the left and
// the file's name after it, at a scale of one.
const double MENU_INDENT = 24.0;

// OPTIONS_PAGES / OPTIONS_COUNT / OPTIONS_FILES / OPTIONS_SHEET
//   constants: the Options window's pages, in the order its list shows
// them; how many there are; the page of the settings files; and the first
// of the pages of the sheet's own settings.
const char* const OPTIONS_PAGES[] = {
    "Environment", "Windows", "Adding rows", "Settings files",
    "Sheet", "Sheet: naming", "Sheet: the grid", "Sheet: colours",
    "Sheet: rows and columns", "Sheet: the header" };
const int OPTIONS_COUNT = 10;
const int OPTIONS_FILES = 3;
const int OPTIONS_SHEET = 4;

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

// coloured
//   text in a colour, on a panel: the colour as far as it reads there, and
// the theme's text for no colour -- a row with no fill. The panel and the
// text are the scheme's, as the window is drawn with them.
element_template
coloured(
    const model&       _model,
    const std::string& _text,
    rgba               _colour
)
{
    const scheme_colours surfaces = scheme_for(_model.ui.theme,
                                               _model.ui.scheme);
    const ImVec4         shown    = ImGui::ColorConvertU32ToFloat4(
                                        legible(_colour,
                                                to_u32(surfaces.panel),
                                                to_u32(surfaces.text)));

    return label(_text, attrs({ { "r", static_cast<double>(shown.x) },
                                { "g", static_cast<double>(shown.y) },
                                { "b", static_cast<double>(shown.z) },
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

// form_heading
//   a heading between a form's parts: the part's name in the labels'
// column, and a faint line beside it.
element_template
form_heading(
    const std::string& _text
)
{
    return table_row({ label(_text, attrs({ { "tone", "accent" } })),
                       separator() });
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

/*
names_by_items
  The menu of what the symbols column lists each row by, as the buttons
over the column choose it: its symbol name, its own name, or its value as
the header writes it; then the forms that write every row's value alike,
lined up -- the two that need an integer's width greyed in a wider sheet,
and the two least plain with an example. The one chosen is checked; each
is one "set" of "grid.names_by", one step to undo.
*/
templates
names_by_items(
    const model& _model
)
{
    const setting* found  = find_setting("grid.names_by");
    const long     chosen = (found != nullptr)
                                ? static_cast<long>(found->read(_model.doc))
                                : 1L;
    const bool     narrow = !is_wide(_model.doc.width);
    const long     values = static_cast<long>(NAMES_BY_VALUE);

    // choice
    //   one way to list the rows, checked while it is the one.
    auto choice = [chosen](const char* _text,
                           long        _value,
                           const char* _example,
                           bool        _enabled)
    {
        return menu_item(_text, "set",
                         attrs({ { "target", "grid.names_by" },
                                 { "value", _value },
                                 { "checkable", true },
                                 { "checked", chosen == _value },
                                 { "shortcut", _example },
                                 { "enabled", _enabled } }));
    };

    return {
        label("list each row by", attrs({ { "tone", "muted" } })),
        choice("Symbol name", 0L, "", true),
        choice("Name", 1L, "", true),
        choice("Value, as the header writes it", values, "", true),
        separator(),
        label("values lined up", attrs({ { "tone", "muted" } })),
        choice("Hex", values + 1L, "", true),
        choice("Decimal", values + 2L, "", narrow),
        choice("Binary", values + 3L, "", narrow),
        choice("Shifted run", values + 4L, "0x3 << 4", true),
        choice("Bits set", values + 5L, "bits 5:4", true) };
}

// setting_toggle
//   a checkable menu item over a flag setting of the sheet's.
element_template
setting_toggle(
    const model& _model,
    const char*  _text,
    const char*  _path,
    const char*  _shortcut = ""
)
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
}

/*
sort_items
  The menu of the order the grid and the tree show each group's rows in:
the sheet's own, or sorted by a key, the one chosen checked; then which
way; and the sorted order made the sheet's own, which the header follows.
Each choice is one "set" of "grid.sort_by" or "grid.sort_descending", one
step to undo.
*/
templates
sort_items(
    const model& _model
)
{
    const grid_settings& grid   = _model.doc.grid;
    const long           chosen = static_cast<long>(grid.sort_by);
    const bool           sorted = (grid.sort_by != sort_key::sheet);

    // choice
    //   one key, checked while it is the one.
    auto choice = [chosen](const char* _text,
                           sort_key    _key)
    {
        const long value = static_cast<long>(_key);

        return menu_item(_text, "set",
                         attrs({ { "target", "grid.sort_by" },
                                 { "value", value },
                                 { "checkable", true },
                                 { "checked", chosen == value } }));
    };

    return {
        label("sort each group's rows by", attrs({ { "tone", "muted" } })),
        choice("The sheet's own order", sort_key::sheet),
        separator(),
        choice("Symbol name", sort_key::symbol),
        choice("Name", sort_key::name),
        choice("Short name", sort_key::short_name),
        choice("Value", sort_key::value),
        choice("Lowest bit", sort_key::lowest),
        choice("Width", sort_key::width),
        choice("Kind", sort_key::kind),
        choice("Note", sort_key::note),
        separator(),
        menu_item("Descending", "set",
                  attrs({ { "target", "grid.sort_descending" },
                          { "value", !grid.sort_descending },
                          { "checkable", true },
                          { "checked", grid.sort_descending },
                          { "enabled", sorted } })),
        menu_item("Make it the sheet's own order", "grid.sort_keep",
                  attrs({ { "value", 0L },
                          { "enabled", sorted } })) };
}

// SHOWN_NAMES
//   constant: what a display rule sets, as shown_part numbers them: as a
// menu calls each, and as the Display window's columns do.
const char* const SHOWN_NAMES[SHOWN_PART_COUNT] = {
    "Digits in cells", "Names across ranges", "Colours" };

// ids_text
//   rows by id, as a display event carries them: "3,7,12".
std::string
ids_text(
    const std::vector<int>& _rows
)
{
    std::string out;

    for (int each : _rows)
    {
        out += (out.empty() ? "" : ",") + format_dec(static_cast<u64>(each));
    }

    return out;
}

// bits_text
//   a rule's or a region's bits, as the menus and the Display window write
// them.
std::string
bits_text(
    int _lsb,
    int _msb
)
{
    if (_msb < _lsb)
    {
        return "every bit";
    }

    if (_msb == _lsb)
    {
        return "bit " + format_dec(static_cast<u64>(_lsb));
    }

    return "bits " + format_dec(static_cast<u64>(_msb)) + ".." +
           format_dec(static_cast<u64>(_lsb));
}

// rows_text
//   a rule's or a region's rows, as the Display window writes them: their
// symbol names, the first few.
std::string
rows_text(
    const model&            _model,
    const std::vector<int>& _rows
)
{
    if (_rows.empty())
    {
        return "every row";
    }

    std::string out;
    std::size_t named = 0;

    for (int each : _rows)
    {
        if (named == 3)
        {
            out += " and " + format_dec(static_cast<u64>(_rows.size() - 3)) +
                   " more";

            break;
        }

        // a symbol by its symbol name; a group by its own
        const resolved_row* found = _model.res.find(each);
        const std::string   name  =
            (found == nullptr)                  ? "(gone)"
          : ( (found->kind == row_kind::group) ||
              (found->full.empty()) )           ? found->local
                                                : found->full;

        out += (out.empty() ? "" : ", ") + name;
        ++named;
    }

    return out;
}

// lineage_in
//   a row's lineage, from the resolution: the row's id, then each group's
// it is in.
std::vector<int>
lineage_in(
    const model& _model,
    int          _id
)
{
    std::vector<int>    out;
    const resolved_row* at = _model.res.find(_id);

    while (at != nullptr)
    {
        out.push_back(at->id);
        at = _model.res.find(at->parent);
    }

    return out;
}

// scope_shows
//   whether a region of the grid shows a part: 0 nowhere, 1 in places, 2
// everywhere -- read over its rows, at every bit where a rule begins or
// ends, between which nothing changes.
int
scope_shows(
    const model&            _model,
    const std::vector<int>& _rows,
    int                     _lsb,
    int                     _msb,
    shown_part              _part
)
{
    const grid_settings& grid  = _model.doc.grid;
    const int            low   = (_msb < _lsb) ? 0 : _lsb;
    const int            high  = (_msb < _lsb) ? (_model.doc.width - 1)
                                               : _msb;
    std::vector<int>     marks = { low };

    for (const display_rule& each : grid.rules)
    {
        if (each.msb < each.lsb)
        {
            continue;
        }

        for (int bit : { each.lsb, each.msb + 1 })
        {
            if ( (bit > low) &&
                 (bit <= high) )
            {
                marks.push_back(bit);
            }
        }
    }

    std::vector<int> ids = _rows;

    if (ids.empty())
    {
        for (const resolved_row& each : _model.res.rows)
        {
            ids.push_back(each.id);
        }
    }

    bool any   = false;
    bool every = true;

    for (int id : ids)
    {
        const std::vector<const display_rule*> over =
            rules_over(grid, lineage_in(_model, id));

        for (int bit : marks)
        {
            const bool on = shows(shown_at(grid, over, bit, bit), _part);

            any   = ( (any) || (on) );
            every = ( (every) && (on) );
        }
    }

    return every ? 2 : (any ? 1 : 0);
}

/*
display_items
  The menu of what a region of the grid shows: its digits, the names across
its ranges and its colours, each checked where the region shows it all --
"in part" where it shows it in places -- and a press shows or hides it
there; then each shown there and nowhere else; and the region as the rest
of the grid shows it, its own rules taken back. Each is one
"display.set", one step to undo.
*/
templates
display_items(
    const model&            _model,
    const std::vector<int>& _rows,
    int                     _lsb,
    int                     _msb,
    const std::string&      _what
)
{
    const std::string rows  = ids_text(_rows);
    templates         items = { label(_what + ", " + bits_text(_lsb, _msb),
                                      attrs({ { "tone", "muted" } })) };
    bool              own   = false;

    // whether a rule is over just this region: it can be taken back
    std::vector<int> sorted = _rows;

    std::sort(sorted.begin(), sorted.end());

    for (const display_rule& each : _model.doc.grid.rules)
    {
        std::vector<int> held = each.rows;

        std::sort(held.begin(), held.end());

        own = ( (own) ||
                ( (held == sorted) &&
                  ( ( (each.msb < each.lsb) && (_msb < _lsb) ) ||
                    ( (each.lsb == _lsb) && (each.msb == _msb) ) ) ) );
    }

    // event
    //   a display.set over the region.
    auto event = [&](shown_part _part,
                     long       _value,
                     bool       _only)
    {
        return attrs({ { "rows", rows },
                       { "lsb", static_cast<long>(_lsb) },
                       { "msb", static_cast<long>(_msb) },
                       { "part", static_cast<long>(_part) },
                       { "value", _value },
                       { "only", _only } });
    };

    for (int i = 0; i < SHOWN_PART_COUNT; ++i)
    {
        const shown_part part  = static_cast<shown_part>(i);
        const int        state = scope_shows(_model, _rows, _lsb, _msb, part);
        const std::string text = std::string(SHOWN_NAMES[i]) +
                                 ((state == 1) ? " (in part)" : "");

        items.push_back(menu_item(text, "display.set",
                                  ::djinterp::overlay(
                                      attrs({ { "checkable", true },
                                              { "checked", state == 2 } }),
                                      event(part,
                                            (state == 2) ? 0L : 1L,
                                            false))));
    }

    items.push_back(separator());

    for (int i = 0; i < SHOWN_PART_COUNT; ++i)
    {
        const shown_part part = static_cast<shown_part>(i);

        items.push_back(menu_item(std::string(SHOWN_NAMES[i]) +
                                      " here only",
                                  "display.set",
                                  event(part, 1L, true)));
    }

    items.push_back(separator());
    items.push_back(menu_item("As the rest of the grid", "display.reset",
                              ::djinterp::overlay(
                                  attrs({ { "enabled", own } }),
                                  event(shown_part::digits, -1L, false))));
    items.push_back(menu_item("Every display rule...", "tool.open",
                              attrs({ { "value",
                                        static_cast<long>(tool::display) } })));

    return items;
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

// file_name
//   a document's file, without the directories it is in: what its tab and
// the menus call it.
std::string
file_name(
    const model& _model
)
{
    const std::string& path  = _model.ui.path;
    const std::size_t  slash = path.find_last_of("/\\");
    const std::string  name  = (slash == std::string::npos)
                                   ? path
                                   : path.substr(slash + 1);

    return name.empty() ? std::string("untitled") : name;
}

// look_items
//   the Theme menu: the themes, then the colour schemes, each checked when
// it is the window's; then the sheet's own colours.
templates
look_items(
    const shell_view& _view
)
{
    const shell_state& shell = _view.shell;
    templates          looks;

    for (int i = 0; i < THEME_COUNT; ++i)
    {
        looks.push_back(menu_item(theme_name(i), "view.theme",
                                  attrs({ { "value", static_cast<long>(i) },
                                          { "checkable", true },
                                          { "checked", shell.theme == i } })));
    }

    looks.push_back(separator("Colour scheme"));

    for (int i = 0; i < SCHEME_COUNT; ++i)
    {
        looks.push_back(menu_item(scheme_name(i), "view.scheme",
                                  attrs({ { "value", static_cast<long>(i) },
                                          { "checkable", true },
                                          { "checked", shell.scheme == i } })));
    }

    // whether the sheet chooses any colour the scheme would
    const colour_settings& own   = _view.front.doc.colours;
    const grid_settings&   lines = _view.front.doc.grid;
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
    looks.push_back(menu_item("The sheet's colours...", "tool.open",
                              attrs({ { "value", static_cast<long>(
                                                     tool::colours) } })));
    looks.push_back(menu_item("Give the sheet the scheme's colours",
                              "colours.automatic",
                              attrs({ { "enabled", owned } })));

    return looks;
}

// example_items
//   the examples, on their shelves in the order the shelves first appear,
// each with its size beside it.
templates
example_items()
{
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

    return examples;
}

/*
menus
  The menu bar: File, Edit, View, Window and Tools, then -- after an indent
-- the workspace in front, chosen from the workspaces, the sheet in front's
file and what the window last said; and Help at its right end. View lists
every tool window, each checked while it is open in this workspace;
Window holds the workspaces, the sheets of this one and how they are laid
out.
*/
element_template
menus(
    const shell_view& _view
)
{
    const shell_state& shell  = _view.shell;
    const model&       front  = _view.front;
    const workspace&   here   = shell.workspaces[shell.current];
    const long         spaces = static_cast<long>(shell.workspaces.size());

    // view_toggle
    //   a checkable menu item over a flag setting.
    auto view_toggle = [&front](const char* _text,
                                const char* _path,
                                const char* _shortcut)
    {
        return setting_toggle(front, _text, _path, _shortcut);
    };

    const bool     selected = !front.ui.selection.empty();
    const long     first    = selected ? static_cast<long>(
                                             front.ui.selection.front())
                                       : 0L;
    const openness groups   = openness_of(front.doc);

    // the tool windows, each checked while it is open here
    templates tools;

    for (int i = 0; i < TOOL_COUNT; ++i)
    {
        const std::size_t at = static_cast<std::size_t>(i);

        tools.push_back(menu_item(
            (i == 0) ? std::string("Symbols tree")
                     : std::string(tool_title(static_cast<tool>(i))),
            "tool.toggle",
            attrs({ { "value", static_cast<long>(i) },
                    { "checkable", true },
                    { "checked", here.tools[at] } })));

        // the two beside the sheets, the two among them, the rest below
        if ( (i == 1) ||
             (i == 3) )
        {
            tools.push_back(separator());
        }
    }

    // the workspaces, to switch to or to move the sheet in front to
    templates switch_to;
    templates move_to;

    for (long i = 0; i < spaces; ++i)
    {
        const std::string& name =
            shell.workspaces[static_cast<std::size_t>(i)].name;
        const bool         now  =
            (static_cast<std::size_t>(i) == shell.current);

        switch_to.push_back(menu_item(name, "workspace.switch",
                                      attrs({ { "value", i },
                                              { "checkable", true },
                                              { "checked", now } })));
        move_to.push_back(menu_item(name, "workspace.move",
                                    attrs({ { "value", i },
                                            { "enabled", !now } })));
    }

    move_to.push_back(separator());
    move_to.push_back(menu_item("A new workspace", "workspace.move",
                                attrs({ { "value", spaces } })));

    // the sheets of this workspace, the one in front checked
    templates sheets;
    const bool several = (_view.here.size() > 1);

    for (const model* each : _view.here)
    {
        sheets.push_back(menu_item(file_name(*each) +
                                       (each->ui.dirty ? " *" : ""),
                                   "doc.activate",
                                   attrs({ { "value", static_cast<long>(
                                                          each->id) },
                                           { "checkable", true },
                                           { "checked",
                                             each->id == front.id } })));
    }

    // the add menu's kinds, two with their keys
    templates adding;

    for (int i = 0; i < ADD_KINDS; ++i)
    {
        const int kind = add_kind_of(i);

        if (kind < 0)
        {
            adding.push_back(separator());
        }

        adding.push_back(menu_item(add_kind_name(i), "edit.add",
                                   attrs({ { "value", static_cast<long>(kind) },
                                           { "shortcut",
                                             (kind == 0)  ? "Ctrl+B"
                                           : (kind == -1) ? "Ctrl+G"
                                                          : "" } })));
    }

    // the workspaces, as the combo before the file's name lists them: the
    // last choice is a new one
    std::vector<std::string> names;

    for (const workspace& each : shell.workspaces)
    {
        names.push_back(each.name);
    }

    names.push_back("+ New workspace");

    return menu_bar({
        menu("File", {
            menu_item("New", "file.new",
                      attrs({ { "shortcut", "Ctrl+N" } })),
            menu_item("Open sheet, bitfield or header", "file.open",
                      attrs({ { "shortcut", "Ctrl+O" } })),
            menu_item("Close", "doc.close",
                      attrs({ { "shortcut", "Ctrl+W" } })),
            separator(),
            menu_item("Import definition files...", "definitions.import"),
            menu_item("Remove the definition file in front",
                      "definitions.remove",
                      attrs({ { "enabled", front.ui.definition >= 0 } })),
            separator(),
            menu_item("Save", "file.save", attrs({ { "shortcut", "Ctrl+S" } })),
            menu_item("Save As...", "file.save_as",
                      attrs({ { "shortcut", "Ctrl+Shift+S" } })),
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
            menu("Examples", example_items()),
            separator(),
            menu_item("Quit", "app.quit") }),
        menu("Edit", {
            menu_item("Undo", "edit.undo",
                      attrs({ { "shortcut", "Ctrl+Z" },
                              { "enabled", front.steps.can_undo() } })),
            menu_item("Redo", "edit.redo",
                      attrs({ { "shortcut", "Ctrl+Y" },
                              { "enabled", front.steps.can_redo() } })),
            separator(),
            menu("Add", std::move(adding)),
            menu_item("Add node...", "node.open",
                      attrs({ { "shortcut", "Ctrl+Shift+A" } })),
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
            menu("Tool windows", std::move(tools)),
            menu_item("Put the windows below away", "panel.toggle",
                      attrs({ { "shortcut", "Ctrl+J" } })),
            separator(),
            menu("Columns", {
                view_toggle("Symbols", "grid.show_names", ""),
                view_toggle("Value", "grid.show_values", ""),
                view_toggle("Bit/byte visualizer", "grid.show_bits", ""),
                separator(),
                view_toggle("Value before Symbols", "grid.values_first", ""),
                separator(),
                // what the symbols column lists rows by, as the buttons
                // over it choose, and their menu; and the order the rows
                // are shown in
                menu("Symbols by", names_by_items(front)),
                menu("Sort rows by", sort_items(front)) }),
            menu("Grid", {
                view_toggle("Grid lines", "grid.lines", "G"),
                view_toggle("Bit indices", "grid.show_indices", ""),
                view_toggle("Digits in cells", "grid.show_digits", ""),
                view_toggle("Names across ranges", "grid.show_labels", ""),
                view_toggle("Colours in cells", "grid.show_colours", ""),
                view_toggle("Cell boxes", "grid.show_cell_boxes", ""),
                view_toggle("Proportional cells", "grid.square_cells", ""),
                view_toggle("Probe row", "grid.show_probe", ""),
                view_toggle("Usage row", "grid.show_usage", ""),
                view_toggle("Macro names in the grid", "grid.full_names", ""),
                view_toggle("Abbreviate unused runs",
                            "grid.abbreviate_unused", ""),
                separator(),
                menu_item("Display rules...", "tool.open",
                          attrs({ { "value",
                                    static_cast<long>(tool::display) } }))
            }),
            menu_item("Expand all groups", "tree.open_all",
                      attrs({ { "value", 0L },
                              { "shortcut", "Ctrl+Shift+Plus" },
                              { "enabled", groups.closed } })),
            menu_item("Collapse all groups", "tree.close_all",
                      attrs({ { "value", 0L },
                              { "shortcut", "Ctrl+Shift+Minus" },
                              { "enabled", groups.open } })),
            separator(),
            menu_item("Zoom in", "view.zoom_in",
                      attrs({ { "shortcut", "Ctrl+=" } })),
            menu_item("Zoom out", "view.zoom_out",
                      attrs({ { "shortcut", "Ctrl+-" } })),
            menu_item("Actual size", "view.zoom_reset",
                      attrs({ { "shortcut", "Ctrl+0" } })),
            menu_item("Every window at actual size", "view.zoom_reset_all",
                      attrs({ { "enabled", !shell.zoom.empty() } })),
            separator(),
            menu("Theme", look_items(_view)),
            separator(),
            menu_item("ImGui demo", "app.demo",
                      attrs({ { "checkable", true },
                              { "checked", shell.demo } })) }),
        menu("Window", {
            menu_item("New workspace", "workspace.new"),
            menu_item("Name this workspace...", "workspace.naming"),
            menu_item("Close this workspace", "workspace.close",
                      attrs({ { "enabled", spaces > 1 } })),
            menu("Workspaces", std::move(switch_to)),
            menu_item("Next workspace", "workspace.next",
                      attrs({ { "shortcut", "Ctrl+Alt+Right" },
                              { "enabled", spaces > 1 } })),
            menu_item("Previous workspace", "workspace.previous",
                      attrs({ { "shortcut", "Ctrl+Alt+Left" },
                              { "enabled", spaces > 1 } })),
            menu("Move this sheet to", std::move(move_to)),
            separator(),
            menu_item("Sheets as tabs", "window.arrange",
                      attrs({ { "value", 0L } })),
            menu_item("Sheets in columns", "window.arrange",
                      attrs({ { "value", 1L }, { "enabled", several } })),
            menu_item("Sheets in rows", "window.arrange",
                      attrs({ { "value", 2L }, { "enabled", several } })),
            menu_item("Sheets in a grid", "window.arrange",
                      attrs({ { "value", 3L }, { "enabled", several } })),
            menu_item("Reset the layout", "window.reset_layout"),
            separator("Sheets here"),
            menu(here.name, std::move(sheets)),
            menu_item("Close this sheet", "doc.close",
                      attrs({ { "shortcut", "Ctrl+W" } })),
            menu_item("Close every sheet here", "doc.close_all") }),
        menu("Tools", {
            menu_item("Options...", "options.open",
                      attrs({ { "shortcut", "Ctrl+," } })),
            menu_item("Import or export settings...", "options.open",
                      attrs({ { "value", static_cast<long>(
                                             OPTIONS_FILES) } })) }),
        // after the menus, an indent, then the workspace; Help at the bar's
        // right end. The sheet's file is its tab's name, and what the window
        // last said is at the status bar's right end
        menu_bar_rest(
            { combo(names,
                    static_cast<long>(shell.current),
                    "workspace.switch",
                    attrs({ { "width", 150.0 },
                            { "key", "workspace.combo" },
                            { "tooltip", "the workspace in front: its "
                                         "sheets and tool windows, laid "
                                         "out as you left them. Choose "
                                         "another, or a new one "
                                         "(Ctrl+Alt+Left, Ctrl+Alt+Right)" } })) },
            { menu("Help", {
                  menu_item("Keyboard shortcuts", "help.show",
                            attrs({ { "value", 0L } })),
                  separator(),
                  menu_item("About bitwis(e)dom", "help.show",
                            attrs({ { "value", 1L } })) }) },
            attrs({ { "indent", MENU_INDENT },
                    { "key", "menu.rest" } })) });
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

// NO_SWATCH
//   constant: the alpha a tree row's swatch is given when the row has no
// fill: enough for uxoxo to leave the swatch its room, so the name lines up
// with its siblings', and too little for ImGui to draw anything there.
const double NO_SWATCH = 1.0e-3;

// child_attrs
//   an "Add child..." field's attributes: its key, its tip, and -- when the
// window gives it the keyboard, after a child was added or a name refused
// -- that request, and the text to start from.
option_set
child_attrs(
    const model& _model,
    int          _parent
)
{
    option_set made = attrs({ { "key", "add." + format_dec(
                                           static_cast<u64>(_parent)) },
                              { "tooltip", "type a name and press Enter: a "
                                           "bit of that name, at the next "
                                           "bit free here" },
                              { "plus", _model.ui.child_offer != 1 },
                              { "field", _model.ui.child_offer != 2 },
                              { "fill", _model.ui.child_fill } });

    if ( (_model.ui.adding_focus)            &&
         (_model.ui.adding_to == _parent)     &&
         (_model.ui.adding_where == "tree") )
    {
        made.set("focus", true);
        made.set("text", _model.ui.adding_text);
    }

    return made;
}

// children_shown
//   a group's rows in the order the tree shows them: the sheet's own, or
// sorted as the grid is.
std::vector<const row*>
children_shown(
    const model& _model,
    const row&   _row
)
{
    std::vector<const row*> out;

    out.reserve(_row.children.size());

    for (const row& child : _row.children)
    {
        out.push_back(&child);
    }

    if (_model.doc.grid.sort_by != sort_key::sheet)
    {
        std::stable_sort(out.begin(),
                         out.end(),
                         [&_model](const row* _a,
                                   const row* _b)
                         {
                             return (place_of(_model, _a->id) <
                                     place_of(_model, _b->id));
                         });
    }

    return out;
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
        for (const row* child : children_shown(_model, _row))
        {
            children.push_back(tree_row(_model, *child));
        }
    }

    // under the last of an open group's or range's rows, a field to name
    // one more: an empty one has none, or every range would have one
    if ( (group)                  &&
         (!_row.collapsed)        &&
         (!_row.children.empty()) )
    {
        children.push_back(add_child_field(_row.id, child_attrs(_model,
                                                                _row.id)));
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
                              { "a", _row.transparent ? NO_SWATCH : 1.0 },
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

// add_attrs
//   the add control's attributes, as the user prefers it: a button and its
// menu, or a split button and the kind it adds; and the colour new rows
// take, which its menu sets.
option_set
add_attrs(
    const shell_state& _shell,
    const std::string& _key
)
{
    return attrs({ { "style", (_shell.prefs.add == add_style::split)
                                  ? "split"
                                  : "menu" },
                   { "kind", static_cast<long>(_shell.prefs.add_kind) },
                   { "colour", static_cast<long>(_shell.prefs.colour) },
                   { "rgba", static_cast<long>(_shell.prefs.colour_rgba) },
                   { "key", _key },
                   { "tooltip", "add a row: into the selected group or "
                                "range, beside the selected symbol, or at "
                                "the end of the sheet (Ctrl+B a bit, "
                                "Ctrl+G a group)" } });
}

/*
symbols_tool
  The symbols tree, in a window of its own: the add control and the buttons
that open and close every group above the rows; then the rows, ending in a
"+" with its faded "Add new...".
*/
element_template
symbols_tool(
    const shell_view& _view
)
{
    const model& front = _view.front;
    templates    rows;

    for (const row* child : children_shown(front, front.doc.root))
    {
        rows.push_back(tree_row(front, *child));
    }

    rows.push_back(add_new_button(attrs({ { "key", "tree.add" },
                                          { "tooltip", "Add a symbol or "
                                                       "show the Probe and "
                                                       "Allocated rows" } })));

    if (rows.size() == 1)
    {
        rows.push_back(muted("An empty sheet. Add a bit, a range or a "
                             "group with the button above, or open an "
                             "example from the File menu."));
    }

    const openness groups = openness_of(front.doc);

    return column({
        hrow({ add_control(add_attrs(_view.shell, "tree.add_control")),
               action_button("Expand all", "tree.open_all",
                             attrs({ { "value", 0L },
                                     { "small", true },
                                     { "key", "open_all" },
                                     { "tooltip", "open every group, in "
                                       "the tree and the grid "
                                       "(Ctrl+Shift+Plus)" } }),
                             groups.closed),
               action_button("Collapse all", "tree.close_all",
                             attrs({ { "value", 0L },
                                     { "small", true },
                                     { "key", "close_all" },
                                     { "tooltip", "close every group, in "
                                       "the tree and the grid "
                                       "(Ctrl+Shift+Minus)" } }),
                             groups.open) }),
        separator(),
        panel("tree", std::move(rows), attrs({ { "height", 0.0 } })) });
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
    const resolved_row& _resolved,
    const std::string&  _act = "row."
)
{
    const long id    = _row.id;
    const int  top   = _model.doc.width - 1;
    templates  rows;

    // number
    //   an integer field over one of the row's fields.
    auto number = [id, &_act](long               _value,
                       const std::string& _field,
                       long               _low,
                       long               _high)
    {
        return int_field(_value, _act + _field,
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
                                    combo(names, chosen, _act + "field",
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
                                          _act + "encoding",
                                          attrs({ { "target", id },
                                                  { "width", 1.0 } }))));

            if (_row.enc == encoding::offset_binary)
            {
                rows.push_back(form_row(
                    "bias",
                    int_field(static_cast<long>(_row.bias), _act + "bias",
                              attrs({ { "target", id },
                                      { "width", 1.0 },
                                      { "key", "bias" } }))));
            }

            rows.push_back(form_row(
                "number",
                text_field(format_signed(_row.number), _act + "number",
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
                           _act + "literal",
                           attrs({ { "target", id },
                                   { "key", "literal" },
                                   { "tooltip", "0x1F, 31, 037, 0b11111, "
                                                "or a negative number" } }))));
            rows.push_back(table_row({ label(""),
                                       toggle("written as negative",
                                              _row.negative,
                                              _act + "negative",
                                              attrs({ { "target", id } })) }));
            rows.push_back(form_row(
                "least digits",
                int_field(_row.digits, _act + "digits",
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
                text_field(_row.expr, _act + "expr",
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
                                    text_field(_row.params, _act + "params",
                                               attrs({ { "target", id },
                                                       { "key", "params" },
                                                       { "placeholder",
                                                         "x, v" } }))));
            if (_row.params.empty())
            {
                rows.push_back(table_row({
                    label(""),
                    toggle("function-like: NAME()", _row.callable,
                           _act + "callable",
                           attrs({ { "target", id },
                                   { "tooltip", "a macro that takes no "
                                                "arguments but is called "
                                                "with parentheses" } })) }));
            }

            rows.push_back(form_row("body",
                                    text_field(_row.body, _act + "body",
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
    const model&       _model,
    const row&         _row,
    const std::string& _act = "row."
)
{
    const long          id       = _row.id;
    const resolved_row* resolved = _model.res.find(_row.id);
    const rgba          colour   = (resolved != nullptr) ? resolved->colour
                                                         : _row.colour;

    // text
    //   a text field over one of the row's fields.
    auto text = [id, &_act](const std::string& _value,
                     const std::string& _field,
                     const std::string& _tip)
    {
        return text_field(_value, _act + _field,
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
                          _act + "colour",
                          attrs({ { "target", id } })),
              action_button("Auto", _act + "auto_colour",
                            attrs({ { "target", id }, { "small", true } }),
                            _row.colour != 0 || _row.transparent),
              toggle("No fill", _row.transparent, _act + "transparent",
                     attrs({ { "target", id },
                             { "tooltip", "Transparent cells: no row "
                                          "fill or range tint" } })) })));
    rows.push_back(form_row(
        "literals",
        combo({ "as the group", "hex", "decimal", "octal", "binary",
                "shifts" },
              static_cast<long>(_row.fmt) + 1, _act + "radix",
              attrs({ { "target", id }, { "width", 1.0 } }))));
    rows.push_back(form_row(
        "suffix type",
        combo({ "as the group", "none", "u", "ul", "ull" },
              static_cast<long>(_row.suf) + 1, _act + "int_suffix",
              attrs({ { "target", id }, { "width", 1.0 } }))));
    rows.push_back(table_row({ label(""),
                               toggle("in the header", _row.emit, _act + "emit",
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
    const model&      _model,
    const row&        _row,
    const option_set& _add
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

    // a range holds rows: the add control adds into it
    templates actions;

    if (_row.def == def_kind::range)
    {
        actions.push_back(add_control(_add));
    }

    // one form, top to bottom: the definition, then the value it comes to,
    // then its naming and format, each part under a heading of its own
    top.push_back(form_heading("value"));

    for (element_template& each : value_rows(_model, *resolved))
    {
        top.push_back(std::move(each));
    }

    top.push_back(table_row({ label(""),
                              action_button("Into the probe", "probe.row",
                                            attrs({ { "value",
                                                      static_cast<long>(
                                                          _row.id) },
                                                    { "small", true } }),
                                            resolved->has_value) }));
    top.push_back(form_heading("naming and format"));

    for (element_template& each : common_rows(_model, _row))
    {
        top.push_back(std::move(each));
    }

    return column({
        coloured(_model,
                 resolved->full.empty() ? "(unnamed)" : resolved->full,
                 resolved->colour),
        issue_list(_model, _row.id),
        shared_form("inspector", std::move(top)),
        column(std::move(actions)),
        separator(),
        hrow({ action_button("Duplicate", "edit.duplicate"),
              action_button("Delete", "edit.delete"),
              action_button("Up", "edit.move_up"),
              action_button("Down", "edit.move_down") }) });
}

element_template
group_inspector(
    const model&      _model,
    const row&        _row,
    const option_set& _add
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

    // then the union of its symbols, and its naming and format: one form,
    // each part under a heading of its own
    top.push_back(form_heading("union of its symbols"));

    for (element_template& each : value_rows(_model, *resolved))
    {
        top.push_back(std::move(each));
    }

    top.push_back(form_heading("naming and format"));

    for (element_template& each : common_rows(_model, _row))
    {
        top.push_back(std::move(each));
    }

    return column({
        coloured(_model,
                 "group " + (_row.name.empty() ? std::string("(unnamed)")
                                               : _row.name),
                 resolved->colour),
        muted(format_dec(static_cast<u64>(resolved->children)) +
              (_row.name_children
                   ? " rows; group name prefixes descendants"
                   : " rows; group name is not inherited")),
        shared_form("inspector", std::move(top)),
        separator(),
        hrow({ add_control(_add),
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
        coloured(_model, _model.doc.title, _model.shown.colours.accent),
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

/*
inspector_tool
  The inspector, in a window of its own: the row selected first, the group
it is, or the sheet when nothing is selected.
*/
element_template
inspector_tool(
    const shell_view& _view
)
{
    const model&     front    = _view.front;
    const option_set add      = add_attrs(_view.shell, "inspector.add");
    const row*       selected = inspected(front);
    element_template content  =
        (selected == nullptr)                 ? sheet_inspector(front)
      : (selected->kind == row_kind::group)   ? group_inspector(front,
                                                                *selected,
                                                                add)
                                              : symbol_inspector(front,
                                                                 *selected,
                                                                 add);

    if (front.ui.selection.size() > 1)
    {
        content = column({ muted(format_dec(front.ui.selection.size()) +
                                 " rows selected; the first is shown, and "
                                 "operations act on all of them."),
                           std::move(content) });
    }

    return content;
}

// ---------------------------------------------------------- the tool windows


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

    // the cells: proportional, a row is as tall as its share of a cell's
    // width, and the cell height waits, unused, for that to be turned off;
    // the gap and the rounding wait for the cells to be boxes
    const grid_settings& grid  = _model.doc.grid;
    templates            cells = setting_rows(_model, { "grid.fill",
                                                        "grid.square_cells",
                                                        "grid.cell_width" });

    // waiting
    //   a setting's row, enabled only while it has an effect.
    auto waiting = [&_model, &cells](const char* _path,
                                     bool        _enabled)
    {
        const setting* found = find_setting(_path);

        if (found != nullptr)
        {
            cells.push_back(form_row(found->label,
                                     setting_control(
                                         _model,
                                         _path,
                                         attrs({ { "enabled",
                                                   _enabled } }))));
        }

        return;
    };

    waiting("grid.cell_ratio", grid.square_cells);
    waiting("grid.cell_height", !grid.square_cells);
    waiting("grid.cell_gap", grid.show_cell_boxes);
    waiting("grid.rounding", grid.show_cell_boxes);

    for (element_template& each : setting_rows(_model, {
             "grid.name_width", "grid.value_width",
             "grid.indent", "grid.index_every" }))
    {
        cells.push_back(std::move(each));
    }

    return hrow({
        panel("lines",
              { hrow({ setting_control(_model, "grid.lines"),
                       label("major line every"),
                       setting_control(_model, "grid.major_every",
                                       attrs({ { "width", 60.0 } })),
                       label("bits") }),
                setting_control(_model, "grid.lines_past_rows"),
                table("lines",
                      { "line", "show", "thickness", "colour", "style" },
                      std::move(rows),
                      attrs({ { "borders", "rows" },
                              { "key", "table.lines" },
                              { "widths", "110\n44\n0\n52\n90" } })) },
              attrs({ { "width", 560.0 }, { "height", 0.0 } })),
        panel("cells",
              { form("cells", std::move(cells), 120.0) },
              attrs({ { "width", 300.0 }, { "height", 0.0 } })),
        panel("labels",
              { form("labels", setting_rows(_model, {
                         "grid.show_indices", "grid.show_digits",
                         "grid.show_labels", "grid.show_cell_boxes",
                         "grid.show_names", "grid.show_values",
                         "grid.show_bits", "grid.values_first",
                         "grid.show_probe", "grid.show_usage",
                         "grid.full_names" }), 8.0),
                // what the symbols column lists rows by: the buttons over
                // it, as a choice
                form("symbols by",
                     setting_rows(_model, { "grid.names_by" }),
                     80.0) },
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

/*
display_tab
  What the grid shows, and where: its own settings -- digits, the names
across its ranges, its colours -- then each display rule that shows or
hides them in a part of it: its rows, its bits, and each part as the grid
has it, shown or hidden; the last rule over a cell wins. A rule is made
from the grid's menus, on a row, a segment's heading or a block selected.
*/
element_template
display_tab(
    const model& _model
)
{
    // PART_CHOICES
    //   constant: a rule's choices for one part, as its combo lists them;
    // a rule's -1, 1 and 0 in that order.
    static const std::vector<std::string> PART_CHOICES = {
        "as the grid", "shown", "hidden" };

    const std::vector<display_rule>& rules = _model.doc.grid.rules;
    templates                        rows;

    for (std::size_t i = 0; i < rules.size(); ++i)
    {
        const display_rule& each   = rules[i];
        const std::string   prefix = format_dec(i) + ".";
        templates           cells  = { label(rows_text(_model, each.rows)),
                                       label(bits_text(each.lsb,
                                                       each.msb)) };

        for (int part = 0; part < SHOWN_PART_COUNT; ++part)
        {
            const int  value  = rule_part(each, static_cast<shown_part>(part));
            const long choice = (value < 0) ? 0L : ( (value > 0) ? 1L : 2L );

            cells.push_back(combo(PART_CHOICES, choice, "display.edit",
                                  attrs({ { "target",
                                            prefix + format_dec(
                                                static_cast<u64>(part)) },
                                          { "key", prefix + "part" +
                                                   format_dec(
                                                       static_cast<u64>(
                                                           part)) },
                                          { "width", 1.0 } })));
        }

        cells.push_back(action_button("x", "display.remove",
                                      attrs({ { "value",
                                                static_cast<long>(i) },
                                              { "small", true },
                                              { "key", prefix + "remove" },
                                              { "tooltip", "remove the "
                                                           "rule" } })));
        rows.push_back(table_row(std::move(cells)));
    }

    templates rule_list;

    if (rules.empty())
    {
        rule_list.push_back(muted("No rules: every cell shows what the grid "
                                  "does. Right-click a row, a segment's "
                                  "heading, or a block selected with "
                                  "Shift and a drag, and choose Show in..."));
    }
    else
    {
        rule_list.push_back(table("rules",
                                  { "rows", "bits", "digits", "names",
                                    "colours", "" },
                                  std::move(rows),
                                  attrs({ { "borders", "rows" },
                                          { "key", "table.rules" },
                                          { "widths",
                                            "0\n90\n96\n96\n96\n24" } })));
        rule_list.push_back(hrow({ action_button(
            "Clear every rule", "display.clear",
            attrs({ { "value", 0L },
                    { "tooltip", "every cell as the grid shows it" } })) }));
    }

    return hrow({
        panel("display_grid",
              { label("The grid shows", attrs({ { "tone", "heading" } })),
                setting_control(_model, "grid.show_digits"),
                setting_control(_model, "grid.show_labels"),
                setting_control(_model, "grid.show_colours"),
                muted("Rules show or hide these in parts of the grid: a "
                      "row, a group and every row in it, a run of bits, or "
                      "a block of both. The last rule over a cell wins.") },
              attrs({ { "width", 300.0 }, { "height", 0.0 } })),
        panel("display_rules", std::move(rule_list),
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

element_template console_tab(const shell_view&);

/*
tool_content
  A tool window's content: what it shows of the document in front.
*/
element_template
tool_content(
    const shell_view& _view,
    tool              _tool
)
{
    const model& front = _view.front;

    switch (_tool)
    {
        case tool::symbols:    return symbols_tool(_view);
        case tool::inspector:  return inspector_tool(_view);
        case tool::macros:     return macros_tab(front);
        case tool::templates:  return templates_tab(front);
        case tool::operations: return operations_tab(front);
        case tool::issues:     return issues_tab(front);
        case tool::grid_lines: return grid_tab(front);
        case tool::colours:    return colours_tab(front);
        case tool::columns:    return columns_tab(front);
        case tool::naming:     return naming_tab(front);
        case tool::sheet_page: return sheet_tab(front);
        case tool::display:    return display_tab(front);
        case tool::console:    return console_tab(_view);
    }

    return muted("");
}

// tool_caption
//   a tool window's title: the issues' says how many there are.
std::string
tool_caption(
    const model& _model,
    tool         _tool
)
{
    if (_tool != tool::issues)
    {
        return tool_title(_tool);
    }

    const int count = _model.res.errors + _model.res.warnings;

    return "Issues" + ((count > 0)
                           ? " (" + format_dec(static_cast<u64>(count)) + ")"
                           : std::string());
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
        menu_item("Group", "edit.add", attrs({ { "value", -1L } })),
        separator(),
        menu_item("Add node...", "node.open") };
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

            // what it shows -- it, the rows selected with it, or the block
            // selected around it -- and the order rows are shown in
            const bool many = ( (is_selected(_model, ui.menu_row)) &&
                                (ui.selection.size() > 1) );
            const std::string name =
                (resolved == nullptr)     ? r->name
              : (resolved->full.empty())  ? resolved->local
                                          : resolved->full;
            const std::string what =
                many ? format_dec(static_cast<u64>(ui.selection.size())) +
                           " rows selected"
              : ( (can_contain(*r)) &&
                  (!r->children.empty()) ) ? name + " and its rows"
                                           : name;

            items.push_back(separator());
            items.push_back(menu(many ? "Show in the rows selected"
                               : (r->kind == row_kind::group)
                                    ? "Show in this group"
                               : ( (can_contain(*r)) &&
                                   (!r->children.empty()) )
                                    ? "Show in this range"
                                    : "Show in this row",
                                 display_items(_model,
                                               many ? ui.selection
                                                    : std::vector<int>{
                                                          ui.menu_row },
                                               0,
                                               -1,
                                               what)));

            if ( (!ui.grid_rect_rows.empty()) &&
                 (std::find(ui.grid_rect_rows.begin(),
                            ui.grid_rect_rows.end(),
                            ui.menu_row) != ui.grid_rect_rows.end()) )
            {
                items.push_back(menu("Show in the block selected",
                                     display_items(_model,
                                                   ui.grid_rect_rows,
                                                   ui.grid_rect_low,
                                                   ui.grid_rect_high,
                                                   format_dec(
                                                       static_cast<u64>(
                                                           ui.grid_rect_rows
                                                               .size())) +
                                                       " rows")));
            }

            items.push_back(menu("Sort rows by", sort_items(_model)));
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

        // what its bits show, in every row
        items.push_back(separator());
        items.push_back(menu("Show in these bits",
                             display_items(_model,
                                           {},
                                           ui.menu_lsb,
                                           ui.menu_msb,
                                           "every row")));
    }
    else if (ui.menu_kind == "special")
    {
        // a right-click on a probe or usage row: where it is shown, and how
        // many there are
        const special_row* one = find_special(_model.doc.grid,
                                              ui.menu_special);

        if (one != nullptr)
        {
            const long id    = one->id;
            const bool probe = (one->kind == special_kind::probe);

            // place
            //   a "special.place" item, checked where the row is.
            auto place = [&items, id, one](const char*   _label,
                                           special_place _where)
            {
                items.push_back(menu_item(
                    _label,
                    "special.place",
                    attrs({ { "value", id },
                            { "place", std::string(place_name(_where)) },
                            { "parent", 0L },
                            { "before", 0L },
                            { "ahead", 0L },
                            { "checkable", true },
                            { "checked", one->place == _where },
                            { "enabled", ( (one->place != _where) ||
                                           (_where !=
                                            special_place::tree) ) } })));

                return;
            };

            items.push_back(label(probe ? "the probe" : "the usage",
                                  attrs({ { "tone", "muted" } })));
            items.push_back(separator());
            place("Pin to the top", special_place::top);
            place("Pin to the bottom", special_place::bottom);
            place("Among the rows", special_place::tree);
            items.push_back(label(probe ? "drag it by its name to place it "
                                          "among the rows"
                                        : "drag it by its name into a group "
                                          "to count only the group's",
                                  attrs({ { "tone", "muted" } })));
            items.push_back(separator());
            items.push_back(menu_item(probe ? "Add another probe row"
                                            : "Add another usage row",
                                      "special.duplicate",
                                      attrs({ { "value", id } })));
            items.push_back(menu_item("Remove this row", "special.remove",
                                      attrs({ { "value", id } })));

            if (probe)
            {
                items.push_back(separator());
                items.push_back(menu_item("Clear the probe", "probe.clear",
                                          attrs({ { "value", 0L } })));
            }
        }
    }
    else if (ui.menu_kind == "names_by")
    {
        // a right-click on a button over the symbols column
        items = names_by_items(_model);
        items.push_back(separator());
        items.push_back(menu("Sort rows by", sort_items(_model)));
    }
    else if (ui.menu_kind == "columns")
    {
        // a right-click on the symbols and value columns' heading: the
        // order they show the rows in, what they list them by, and which
        // of them are shown
        items = sort_items(_model);
        items.push_back(separator());
        items.push_back(menu("Symbols by", names_by_items(_model)));
        items.push_back(separator());
        items.push_back(setting_toggle(_model, "Symbols column",
                                       "grid.show_names"));
        items.push_back(setting_toggle(_model, "Value column",
                                       "grid.show_values"));
        items.push_back(setting_toggle(_model, "Value before Symbols",
                                       "grid.values_first"));
    }
    else
    {
        if (ui.menu_where == "add")
        {
            for (element_template& each : add_items())
                items.push_back(std::move(each));
            items.push_back(separator());
            items.push_back(menu_item("Show probe", "set",
                attrs({ { "target", "grid.show_probe" },
                        { "value", !_model.doc.grid.show_probe },
                        { "checkable", true },
                        { "checked", _model.doc.grid.show_probe } })));
            items.push_back(menu_item("Show allocated", "set",
                attrs({ { "target", "grid.show_usage" },
                        { "value", !_model.doc.grid.show_usage },
                        { "checkable", true },
                        { "checked", _model.doc.grid.show_usage } })));
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
        items.push_back(separator());
        items.push_back(menu("Sort rows by", sort_items(_model)));
        items.push_back(menu("Show everywhere", {
            setting_toggle(_model, "Digits in cells", "grid.show_digits"),
            setting_toggle(_model, "Names across ranges",
                           "grid.show_labels"),
            setting_toggle(_model, "Colours", "grid.show_colours"),
            separator(),
            menu_item("Every display rule...", "tool.open",
                      attrs({ { "value",
                                static_cast<long>(tool::display) } })),
            menu_item("Clear the display rules", "display.clear",
                      attrs({ { "value", 0L },
                              { "enabled",
                                !_model.doc.grid.rules.empty() } })) }));
        }
    }

    return popup("context",
                 std::move(items),
                 attrs({ { "open", ui.menu_open },
                         { "key", "context" } }));
}

// --------------------------------------------------------- the dock windows

// slot_name
//   a place in a new workspace's layout, as the dock space's layout names
// it.
const char*
slot_name(
    dock_slot _slot
)
{
    switch (_slot)
    {
        case dock_slot::left:   return "left";
        case dock_slot::right:  return "right";
        case dock_slot::bottom: return "bottom";
        case dock_slot::top:    break;
    }

    return "top";
}

// dock_text
//   a dock space's id as its attribute carries it: text, which holds every
// id a 32-bit hash makes on every platform.
std::string
dock_text(
    const workspace& _workspace
)
{
    return format_dec(static_cast<u64>(dock_id(_workspace.id)));
}

// definition_key
//   the key of a document's window for one of its definition files.
std::string
definition_key(
    int         _document,
    std::size_t _index
)
{
    return document_key(_document) + ".def" +
           format_dec(static_cast<u64>(_index));
}

/*
document_window
  A document's window: its bit grid, flush, under a tab with its file's
name and a dot while it has changes. It docks beside the document before
it the first time it is shown -- or in the middle -- and among this
workspace's documents at once when it has just come from another.
*/
element_template
document_window(
    const shell_view&  _view,
    const model&       _document,
    const std::string& _with
)
{
    const shell_state& shell   = _view.shell;
    const workspace&   here    = shell.workspaces[shell.current];
    const std::string  key     = document_key(_document.id);
    const long         id      = static_cast<long>(_document.id);
    const bool         front   = (_document.id == _view.front.id);
    const bool         placed  =
        (std::find(here.placed.begin(), here.placed.end(), _document.id) !=
         here.placed.end());
    const bool         ops     =
        here.tools[static_cast<std::size_t>(tool::operations)];
    const std::string  preview =
        ( (front) && (ops) ) ? tool_window_key(here.id, tool::operations)
                             : std::string();

    return dock_window(key,
                       file_name(_document),
                       { bit_grid(attrs({ { "document", id },
                                          { "preview_window", preview },
                                          { "front", front },
                                          { "key", "grid" } })) },
                       attrs({ { "dockspace", dock_text(here) },
                               { "dock_with", _with },
                               { "place", placed },
                               { "focus", here.raise == _document.id },
                               { "zoom", static_cast<double>(
                                             zoom_of(shell, key)) },
                               { "zoom_action", "view.zoom" },
                               { "close_action", "doc.close" },
                               { "focus_action", "doc.activate" },
                               { "value", id },
                               { "document", id },
                               { "unsaved", _document.ui.dirty },
                               { "scroll", false },
                               { "flush", true },
                               { "column_up", true },
                               { "rename_action", "doc.rename" },
                               { "renaming", _document.ui.file_renaming },
                               { "rename_focus",
                                 _document.ui.file_rename_focus },
                               { "rename_text", file_name(_document) },
                               { "rename_commit", "doc.rename.commit" },
                               { "rename_cancel", "doc.rename.cancel" } }));
}

/*
definition_window
  One of a document's imported definition files, beside the document: its
x removes the file from the sheet, a step to undo.
*/
element_template
definition_window(
    const shell_view& _view,
    const model&      _document,
    std::size_t       _index
)
{
    const shell_state&       shell  = _view.shell;
    const workspace&         here   = shell.workspaces[shell.current];
    const definition_source& source = _document.doc.definitions[_index];
    const std::string        key    = definition_key(_document.id, _index);
    const long               id     = static_cast<long>(_document.id);
    const std::size_t        slash  = source.path.find_last_of("/\\");
    std::string              name   = (slash == std::string::npos)
                                          ? source.path
                                          : source.path.substr(slash + 1);

    if (name.empty())
    {
        name = "definitions " + format_dec(static_cast<u64>(_index) + 1);
    }

    return dock_window(key,
                       name,
                       { definition_view(attrs({
                             { "document", id },
                             { "value", static_cast<long>(_index) },
                             { "key", "definition" } })) },
                       attrs({ { "dockspace", dock_text(here) },
                               { "dock_with", document_key(_document.id) },
                               { "focus",
                                 ( (_document.ui.definition_raise) &&
                                   (_document.ui.definition ==
                                    static_cast<int>(_index)) ) },
                               { "zoom", static_cast<double>(
                                             zoom_of(shell, key)) },
                               { "zoom_action", "view.zoom" },
                               { "close_action", "definitions.remove" },
                               { "focus_action", "definitions.front" },
                               { "value", static_cast<long>(_index) },
                               { "document", id } }));
}

/*
tool_window
  A tool window of the workspace in front, showing the document in front:
its x closes it, and View opens it again where it was.
*/
element_template
tool_window(
    const shell_view& _view,
    tool              _tool
)
{
    const shell_state& shell = _view.shell;
    const workspace&   here  = shell.workspaces[shell.current];
    const std::string  key   = tool_window_key(here.id, _tool);
    const std::size_t  index = static_cast<std::size_t>(_tool);

    return dock_window(key,
                       tool_caption(_view.front, _tool),
                       { tool_content(_view, _tool) },
                       attrs({ { "dockspace", dock_text(here) },
                               { "focus", here.focus[index] },
                               { "zoom", static_cast<double>(
                                             zoom_of(shell, key)) },
                               { "zoom_action", "view.zoom" },
                               { "close_action", "tool.close" },
                               { "value", static_cast<long>(index) },
                               { "hscroll", true } }));
}

// ---------------------------------------------------------------- options

// env_control
//   the control that edits one of the window's own settings.
element_template
env_control(
    const shell_state& _shell,
    const std::string& _path
)
{
    const env_setting* found = find_env_setting(_path);

    if (found == nullptr)
    {
        return muted("(no setting " + _path + ")");
    }

    const double     value = found->read(_shell);
    const option_set base  = attrs({ { "target", _path },
                                     { "key", _path },
                                     { "tooltip", found->help } });

    switch (found->kind)
    {
        case setting_kind::flag:
            return toggle(found->label, value != 0.0, "set", base);

        case setting_kind::choice:
            return combo(found->choices,
                         static_cast<long>(value),
                         "set",
                         ::djinterp::overlay(attrs({ { "width", 1.0 } }),
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

        default:
            break;
    }

    return muted(_path);
}

// page_paths
//   the settings a page of the Options window shows, by path; the
// settings files' page shows none.
std::vector<std::string>
page_paths(
    int _page
)
{
    // COLUMNS
    //   constant: the grid's settings the columns' page shows, not the
    // grid's.
    static const std::vector<std::string> COLUMNS = {
        "grid.sort_by", "grid.sort_descending", "grid.auto_segments",
        "grid.collapsed", "grid.abbreviate_unused", "grid.unused_run" };

    std::vector<std::string> out;

    // under
    //   every sheet setting whose path begins so, but those named.
    auto under = [&out](const std::string&              _head,
                        const std::vector<std::string>& _not)
    {
        for (const setting& each : settings())
        {
            if ( (each.path.compare(0, _head.size(), _head) == 0) &&
                 (std::find(_not.begin(), _not.end(), each.path) ==
                  _not.end()) )
            {
                out.push_back(each.path);
            }
        }

        return;
    };

    switch (_page)
    {
        case 0:
            return { "environment.theme", "environment.scheme",
                     "environment.restore" };

        case 1:
            return { "environment.merged" };

        case 2:
            return { "environment.add", "environment.add_kind",
                     "environment.child", "environment.child_fill",
                     "environment.node_as",
                     "environment.new_colour", "environment.new_rgba" };

        case 4:
            return { "sheet.title", "sheet.width", "sheet.numbering",
                     "sheet.msb_left", "sheet.bytes", "sheet.prefix",
                     "sheet.suffix" };

        case 5:
            return { "sheet.trim_both", "sheet.trim_left",
                     "sheet.trim_right", "sheet.environment" };

        case 6:
            under("grid.", COLUMNS);
            break;

        case 7:
            under("colours.", {});
            break;

        case 8:
            return COLUMNS;

        case 9:
            under("gen.", {});
            break;

        default:
            break;
    }

    return out;
}

// full_label
//   a setting's label where nothing around it says what it is about: a
// grid line's four, as "Lines between bits: thickness".
std::string
full_label(
    const std::string& _path,
    const std::string& _label
)
{
    // LINES
    //   constant: each kind of grid line's key, and what it is called.
    static const char* const LINES[][2] = {
        { "minor", "between bits" },     { "major", "every n bits" },
        { "rows", "between rows" },      { "groups", "above groups" },
        { "segments", "at segment edges" }, { "border", "around the grid" } };

    for (const auto& each : LINES)
    {
        const std::string head = std::string("grid.") + each[0] + ".";

        if (_path.compare(0, head.size(), head) == 0)
        {
            return std::string("Lines ") + each[1] + ": " + _label;
        }
    }

    return _label;
}

// option_rows
//   a form's rows for settings by path, the window's own or the sheet's; a
// flag's label is its own.
templates
option_rows(
    const shell_view&               _view,
    const std::vector<std::string>& _paths
)
{
    templates rows;

    for (const std::string& path : _paths)
    {
        if (is_environment(path))
        {
            const env_setting* found = find_env_setting(path);

            if (found == nullptr)
            {
                continue;
            }

            rows.push_back((found->kind == setting_kind::flag)
                               ? table_row({ label(""),
                                             env_control(_view.shell, path) })
                               : form_row(found->label,
                                          env_control(_view.shell, path)));

            continue;
        }

        const setting* found = find_setting(path);

        if (found == nullptr)
        {
            continue;
        }

        // a flag's label is its own; a line's says which line
        const std::string said = full_label(path, found->label);

        rows.push_back((found->kind == setting_kind::flag)
                           ? table_row({ label(""),
                                         setting_control(
                                             _view.front, path,
                                             attrs({ { "text", said } })) })
                           : form_row(said,
                                      setting_control(_view.front, path)));
    }

    return rows;
}

// lowered
//   a text in lower case, to search without regard to case.
std::string
lowered(
    std::string _text
)
{
    for (char& each : _text)
    {
        each = static_cast<char>(
            std::tolower(static_cast<unsigned char>(each)));
    }

    return _text;
}

// matches
//   whether a setting's label, path or help holds the text searched for.
bool
matches(
    const std::string& _path,
    const std::string& _query
)
{
    std::string said = _path;

    if (const env_setting* own = find_env_setting(_path))
    {
        said += " " + own->label + " " + own->help;
    }
    else if (const setting* found = find_setting(_path))
    {
        said += " " + full_label(_path, found->label) + " " + found->help;
    }

    return (lowered(said).find(lowered(_query)) != std::string::npos);
}

/*
files_page
  The settings files: a file to export the settings to or import them from,
and the files the window keeps, each to apply again, to forget, or to
delete.
*/
templates
files_page(
    const shell_view& _view
)
{
    const shell_state&              shell = _view.shell;
    const std::vector<std::string>& files = shell.prefs.settings_files;
    const bool                      named = !shell.settings_path.empty();
    templates                       rows;

    for (std::size_t i = 0; i < files.size(); ++i)
    {
        const long index = static_cast<long>(i);

        rows.push_back(table_row({
            label(files[i], attrs({ { "wrap", true } })),
            hrow({ action_button("Apply", "settings.apply",
                                 attrs({ { "value", index },
                                         { "small", true },
                                         { "tooltip", "the window and the "
                                           "sheet in front take its "
                                           "settings" } })),
                   action_button("Forget", "settings.forget",
                                 attrs({ { "value", index },
                                         { "small", true },
                                         { "tooltip", "off the list; the "
                                           "file stays where it is" } })),
                   action_button("Delete", "settings.delete",
                                 attrs({ { "value", index },
                                         { "small", true },
                                         { "tooltip", "the file deleted, "
                                           "and off the list" } })) }) }));
    }

    templates page = {
        muted("A settings file holds the window's own settings and the "
              "sheet's preferences: how it is drawn and how its header is "
              "written -- not its title, width or names. Export them, and "
              "import them into another sheet, or another window. The "
              "files are kept on the list below while they are where they "
              "were, until you forget or delete them."),
        hrow({ text_field(shell.settings_path, "settings.path",
                          attrs({ { "key", "settings.path" },
                                  { "width", 360.0 },
                                  { "placeholder",
                                    "my.bitwisedom-settings.json" },
                                  { "tooltip", "the settings file to "
                                               "export to or import "
                                               "from" } })),
               action_button("Export", "settings.export",
                             attrs({ { "tooltip", "write the window's "
                                       "settings and the sheet's "
                                       "preferences to the file" } }),
                             named),
               action_button("Import", "settings.import",
                             attrs({ { "tooltip", "read the file, and "
                                       "apply what it holds: one step to "
                                       "undo, for the sheet's" } }),
                             named) }) };

    if (rows.empty())
    {
        page.push_back(muted("No settings files kept yet."));
    }
    else
    {
        page.push_back(table("settings.files",
                             { "file", "" },
                             std::move(rows),
                             attrs({ { "borders", "rows" },
                                     { "key", "table.settings_files" },
                                     { "widths", "0\n190" } })));
    }

    return page;
}

/*
node_content
  The Add node window's form: what kind of row, where it goes -- in which
group or range, first or last -- its name, and everything else a row has,
as the inspector shows it; then what it comes to, tried on a copy of the
sheet -- its name and value, and the issues it would have -- and the
buttons that add it.
*/
templates
node_content(
    const model& _model
)
{
    const node_draft& n     = _model.ui.node;
    const row&        draft = n.draft;
    const bool        group = (draft.kind == row_kind::group);
    templates         top;

    // where it goes: the sheet's own rows, or a group or range
    const std::vector<int>   places = container_choices(_model.doc);
    std::vector<std::string> names;
    long                     into   = 0;

    for (std::size_t i = 0; i < places.size(); ++i)
    {
        const resolved_row* each = _model.res.find(places[i]);

        names.push_back( (places[i] == 0) ? std::string("the sheet's own rows")
                       : (each == nullptr) ? std::string("?")
                       : (each->full.empty()) ? each->local
                                              : each->full);

        if (places[i] == n.parent)
        {
            into = static_cast<long>(i);
        }
    }

    const std::vector<std::string> kinds(NODE_KINDS,
                                         NODE_KINDS + NODE_KIND_COUNT);

    top.push_back(form_row("kind",
                           combo(kinds,
                                 group ? static_cast<long>(NODE_KIND_COUNT - 1)
                                       : static_cast<long>(draft.def),
                                 "node.kind",
                                 attrs({ { "width", 1.0 },
                                         { "key", "node.kind" } }))));
    top.push_back(form_row("into",
                           combo(names, into, "node.parent",
                                 attrs({ { "width", 1.0 },
                                         { "key", "node.parent" } }))));
    top.push_back(form_row("goes",
                           combo({ "last there", "first there" },
                                 n.first ? 1L : 0L,
                                 "node.first",
                                 attrs({ { "width", 1.0 },
                                         { "key", "node.first" } }))));
    top.push_back(form_row("name",
                           text_field(draft.name, "node.name",
                                      attrs({ { "key", "node.name" },
                                              { "placeholder", n.fresh },
                                              { "tooltip",
                                                "left empty, it is " +
                                                    n.fresh } }))));

    if (!group)
    {
        const resolved_row shown = n.resolves ? n.resolved : resolved_row{};

        for (element_template& each : definition_rows(_model, draft, shown,
                                                       "node."))
        {
            top.push_back(std::move(each));
        }
    }

    // what it comes to
    templates result;

    if (n.resolves)
    {
        const std::string named = n.resolved.full.empty() ? n.resolved.local
                                                          : n.resolved.full;

        result.push_back(coloured(_model, named, n.resolved.colour));

        if ( (!group) &&
             (n.resolved.has_value) )
        {
            result.push_back(shared_form("node.value",
                                         value_rows(_model, n.resolved)));
        }
    }

    for (const issue& each : n.issues)
    {
        result.push_back(level_label(each.message, each.level, true));
    }

    if (!n.refused.empty())
    {
        result.push_back(level_label(n.refused, severity::error, true));
    }

    const bool can = n.refused.empty();

    return {
        muted("A row of any kind, made in full before it is added. Each add "
              "is a step to undo; the window stays open for the next."),
        shared_form("node.definition", std::move(top)),
        section("Naming and format",
                { shared_form("node.common",
                              common_rows(_model, draft, "node.")) }),
        separator("What it comes to"),
        column(std::move(result), 2, attrs({ { "key", "node.result" } })),
        separator(),
        hrow({ action_button("Add", "node.add",
                             attrs({ { "close", false },
                                     { "key", "node.add" },
                                     { "tooltip", "add it, and start the "
                                                  "next" } }),
                             can),
               action_button("Add and close", "node.add",
                             attrs({ { "close", true },
                                     { "key", "node.add_close" } }),
                             can),
               action_button("Close", "node.close",
                             attrs({ { "key", "node.close" } })) }) };
}

/*
console_tab
  The console: every action the user has taken, as the window heard it --
its name and what it carried, the sheet it was done in, and when -- the
latest first. An action done many times in a row -- a drag, a slider, a
name typed -- is one line, with how many times. The list reads the actions
as each frame is drawn: an action taken is listed without the window being
built again.
*/
element_template
console_tab(
    const shell_view&
)
{
    return action_list(attrs({ { "key", "console.list" } }));
}

/*
status_text
  What the status bar says of the sheet in front: its file, its width, its
symbols and its issues, and the row selected.
*/
std::string
status_text(
    const model& _model
)
{
    int symbols = 0;

    for (const resolved_row& each : _model.res.rows)
    {
        symbols += (each.kind == row_kind::symbol) ? 1 : 0;
    }

    std::string text = file_name(_model) + "    " +
                       format_dec(static_cast<u64>(_model.doc.width)) +
                       " bits    " +
                       format_dec(static_cast<u64>(symbols)) +
                       " symbol(s)    ";

    if ( (_model.res.errors == 0) &&
         (_model.res.warnings == 0) )
    {
        text += "no issues";
    }
    else
    {
        text += format_dec(static_cast<u64>(_model.res.errors)) +
                " error(s), " +
                format_dec(static_cast<u64>(_model.res.warnings)) +
                " warning(s)";
    }

    const std::vector<int>& selection = _model.ui.selection;

    if (selection.size() == 1)
    {
        const resolved_row* chosen = _model.res.find(selection.front());

        if (chosen != nullptr)
        {
            text += "    " + (chosen->full.empty() ? chosen->local
                                                   : chosen->full);
        }
    }
    else if (selection.size() > 1)
    {
        text += "    " + format_dec(static_cast<u64>(selection.size())) +
                " rows selected";
    }

    return text;
}

/*
add_node_window
  The Add node window: a tab among the sheets, or a window of its own over
them, as the preferences say -- each its own window to ImGui, so either
keeps where it was put. It shows the document in front's row in the
making.
*/
element_template
add_node_window(
    const shell_view& _view
)
{
    const shell_state& shell = _view.shell;
    const workspace&   here  = shell.workspaces[shell.current];
    const bool         tab   = (shell.prefs.node_as == node_window::tab);
    const std::string  key   = tab ? "node.tab" : "node.window";
    option_set         made  = attrs({
        { "close_action", "node.close" },
        { "focus", shell.node_raise },
        { "zoom", static_cast<double>(zoom_of(shell, key)) },
        { "zoom_action", "view.zoom" },
        { "document", static_cast<long>(_view.front.id) } });

    if (tab)
    {
        made = ::djinterp::overlay(made,
                                   attrs({ { "dockspace", dock_text(here) },
                                           { "dock_with",
                                             document_key(_view.front.id) },
                                           { "hscroll", true } }));
    }
    else
    {
        made = ::djinterp::overlay(made,
                                   attrs({ { "dockable", false },
                                           { "width", 560.0 },
                                           { "height", 640.0 } }));
    }

    return dock_window(key,
                       "Add node",
                       { column(node_content(_view.front), 4,
                                attrs({ { "key", "node" } })) },
                       made);
}

/*
options_window
  The Options: every setting in one place, as Visual Studio's are. A search
field and the pages down the left; the page, or every setting the search
finds, on the right. The window's own settings apply at once and are kept
for the next run; the sheet's are the sheet in front's, saved with it, and
each change is a step to undo.
*/
element_template
options_window(
    const shell_view& _view
)
{
    const shell_state& shell = _view.shell;
    const int          page  = std::clamp(shell.options_page, 0,
                                          OPTIONS_COUNT - 1);
    const bool         found = !shell.options_search.empty();
    templates          nav;
    templates          shown;

    nav.push_back(text_field(shell.options_search, "options.search",
                             attrs({ { "key", "options.search" },
                                     { "placeholder", "Search the settings" },
                                     { "width", 1.0 } })));

    for (int i = 0; i < OPTIONS_COUNT; ++i)
    {
        nav.push_back(selectable(OPTIONS_PAGES[i],
                                 ( (!found) && (i == page) ),
                                 "options.page",
                                 i,
                                 attrs({ { "key", "options.page." +
                                                  format_dec(
                                                      static_cast<u64>(i)) } })));
    }

    if (found)
    {
        // every setting the search finds, under its page's title
        for (int i = 0; i < OPTIONS_COUNT; ++i)
        {
            std::vector<std::string> hits;

            for (const std::string& path : page_paths(i))
            {
                if (matches(path, shell.options_search))
                {
                    hits.push_back(path);
                }
            }

            if (!hits.empty())
            {
                shown.push_back(separator(OPTIONS_PAGES[i]));
                shown.push_back(form("options.found." +
                                         format_dec(static_cast<u64>(i)),
                                     option_rows(_view, hits),
                                     170.0));
            }
        }

        if (shown.empty())
        {
            shown.push_back(muted("No setting is called that."));
        }
    }
    else if (page == OPTIONS_FILES)
    {
        shown = files_page(_view);
    }
    else
    {
        shown.push_back(label(OPTIONS_PAGES[page],
                              attrs({ { "tone", "heading" } })));

        if (page >= OPTIONS_SHEET)
        {
            shown.push_back(muted("The sheet's own, saved with it: " +
                                  _view.front.doc.title + ". Each change "
                                  "is a step to undo."));
        }
        else if (page == 1)
        {
            shown.push_back(muted("Windows dock as Visual Studio's do: drag "
                                  "one by its tab onto another's, or to an "
                                  "edge. Window lays the sheets out, and "
                                  "resets the layout."));
        }

        shown.push_back(form("options.page." +
                                 format_dec(static_cast<u64>(page)),
                             option_rows(_view, page_paths(page)),
                             170.0));

        if (page == 0)
        {
            shown.push_back(hrow({ action_button(
                "Every window at actual size", "view.zoom_reset_all",
                attrs({ { "tooltip", "each window's zoom back to 100%" } }),
                !shell.zoom.empty()) }));
        }
        else if (page == 1)
        {
            shown.push_back(hrow({ action_button(
                "Reset the layout", "window.reset_layout",
                attrs({ { "tooltip", "this workspace's windows where a new "
                                     "workspace has them" } })) }));
        }
    }

    return dock_window(
        "options",
        "Options",
        { hrow({ panel("options.nav", std::move(nav),
                       attrs({ { "width", 190.0 }, { "height", -34.0 } })),
                 panel("options.shown", std::move(shown),
                       attrs({ { "width", 0.0 }, { "height", -34.0 } })) },
               0),
          hrow({ action_button("Close", "options.close") }) },
        attrs({ { "dockable", false },
                { "close_action", "options.close" },
                { "width", 860.0 },
                { "height", 580.0 },
                { "zoom", static_cast<double>(zoom_of(shell, "options")) },
                { "zoom_action", "view.zoom" } }));
}

}  // anonymous namespace


/*
build_ui
  The window: its menu bar; the dock space of the workspace in front, laid
out as a new workspace's the first time it is shown; a window for each of
its documents, and for each definition file they hold; its tool windows
that are open; the Options, when they are; and the one context menu.
*/
element_template
build_ui(
    const shell_view& _view
)
{
    const shell_state& shell = _view.shell;
    const workspace&   here  = shell.workspaces[shell.current];
    std::string        layout;
    std::string        documents;

    // every window of the workspace, in its place in a new workspace's
    // layout; the documents, in order, to be arranged
    for (const model* each : _view.here)
    {
        const std::string name = "###" + document_key(each->id);

        layout    += name + "\ttop\n";
        documents += name + "\n";

        for (std::size_t i = 0; i < each->doc.definitions.size(); ++i)
        {
            layout += "###" + definition_key(each->id, i) + "\ttop\n";
        }
    }

    for (int i = 0; i < TOOL_COUNT; ++i)
    {
        const tool each = static_cast<tool>(i);

        layout += "###" + tool_window_key(here.id, each) + "\t" +
                  slot_name(tool_slot(each)) + "\n";
    }

    const model& front = _view.front;
    templates    whole = { menus(_view),
                          status_bar(attrs({
                              { "left", status_text(front) },
                              { "right", front.ui.status },
                              { "error", front.ui.status_error },
                              { "dirty", front.ui.dirty },
                              { "action", "tool.open" },
                              { "value", static_cast<long>(tool::console) },
                              { "tooltip", "the console: every action so "
                                           "far" },
                              { "key", "status" } })),
                          dock_space(attrs({ { "id", dock_text(here) },
                                             { "layout", layout },
                                             { "reset", here.reset },
                                             { "arrange", static_cast<long>(
                                                              here.arrange) },
                                             { "documents", documents },
                                             { "key", "dock" } })) };
    std::string with;

    for (const model* each : _view.here)
    {
        whole.push_back(document_window(_view, *each, with));
        with = document_key(each->id);

        for (std::size_t i = 0; i < each->doc.definitions.size(); ++i)
        {
            whole.push_back(definition_window(_view, *each, i));
        }
    }

    for (int i = 0; i < TOOL_COUNT; ++i)
    {
        if (here.tools[static_cast<std::size_t>(i)])
        {
            whole.push_back(tool_window(_view, static_cast<tool>(i)));
        }
    }

    if (shell.options_open)
    {
        whole.push_back(options_window(_view));
    }

    if (shell.node_open)
    {
        whole.push_back(add_node_window(_view));
    }

    whole.push_back(context_menu(_view.front));

    return column(std::move(whole), 0, attrs({ { "key", "shell" } }));
}
}  // namespace gui
}  // namespace bitwisedom
