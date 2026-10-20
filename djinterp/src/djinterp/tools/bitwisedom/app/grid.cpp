/*******************************************************************************
* bitwisedom [app]                                                      grid.cpp
*
* The bit grid: one row per symbol, one column per bit.
*   The grid scrolls both ways inside its own child window, with the column
* headings pinned to the top and the name and value columns pinned to the
* left, as a spreadsheet's are. Everything is drawn with the window's draw
* list from one layout, computed each frame: the columns -- a bit each, or
* one for a collapsed segment or an abbreviated run of unused bits -- and
* the rows -- the probe and usage rows pinned at the top, every visible row
* of the tree in the order the sheet's sort shows them, with those placed
* among them, then the add row and those pinned at the bottom, at the
* grid's foot while the rows leave room.
*   GRID LINES are drawn over the cells, each kind under its own switch,
* thickness, colour and style: between bits, every n bits, between rows,
* above groups, at segment edges, and around the whole -- stopping where the
* rows do, unless the sheet runs them down the room below.
*   INTERACTION posts events and changes nothing itself. A click on a name
* selects its row; on a group's arrow, opens or closes it; on a segment's
* heading, collapses or expands it; on a cell, edits the row's bit. A drag
* across a row's cells defines a range, or paints a literal's bits. A
* double-click on a name, or on a segment's heading, renames it where it is
* shown, in a field over it; a right-click opens the menu for the row, the
* segment, the symbols column's heading or the sheet under the mouse. While
* the tree's own window is closed, the symbols column is the tree: a row
* dragged by its name moves in the sheet, and a row selected has an "Add
* child..." field under its group's rows, with a + that opens the Add node
* window. A probe or usage row dragged by its name moves among the rows,
* or is pinned; a usage row in a group counts the group's rows.
*   COLUMNS are resized by their edges, which light up under the mouse: the
* name and value columns' by a drag of the edge after them, which a
* double-click fits to their text; the bits' cells, all at once, by a drag of
* any boundary in the ruler, which a double-click puts back to the default.
* The grid fills its panel: the room it has beyond what its columns need
* goes to the bits' cells, widened alike, or to the name column, as the
* sheet says -- or to nothing, and then the rows end where the columns do.
* PROPORTIONAL CELLS, the default, make every row as tall as the sheet's
* share of a bit's cell's width -- four fifths, or 1 for square: the cells
* still widen to fill the room, until they are as wide as the cell settings
* allow; and a cell width set by hand is the width they take. What a column or a heading cannot show for want
* of room -- a name or a value cut short, a run of bits shown as one column,
* a band's name -- the mouse shows in full.
*   WHAT A CELL SHOWS -- its digit, a range's name across it, its row's
* colour -- is the grid's settings, as the display rules over its row and
* its bits change them. A row with no fill, or a cell whose colour is
* hidden, draws its set bits plain: the theme's text, faintly.
*   TEXT is drawn in the ink that reads on what lies beneath it -- a cell's
* fill, a range's label chip, a selected row, a band -- whatever colour the
* sheet gave it; a row with no fill puts its name on its own ground.
*   The ADD ROW after the symbols is a "+", and a faded "Add new..." beside
* it; a press on either opens the menu of what to add.
*
* path:      /src/djinterp/tools/bitwisedom/app/grid.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./elements.hpp"  // corresponding header

// std
#include <algorithm>  // std::max, std::min
#include <cmath>      // std::fabs, std::floor, std::sqrt
#include <cstddef>    // std::size_t
#include <map>        // std::map
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector
// imgui
#include <imgui.h>                  // ImGui, ImDrawList
#include <imgui_internal.h>         // the window's inner rectangle
#include <misc/cpp/imgui_stdlib.h>  // InputText over std::string
// bitwisedom
#include <djinterp/tools/bitwisedom/resolve.hpp>  // macro_body
#include "./settings.hpp"  // NAMES_BY_VALUE
#include "./theme.hpp"     // to_u32, mono_font, ink_on, legible, composite


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::option_set;
using uxoxo::platform::imgui_frame;

// column
//   struct: one displayed column -- a bit, or a run of bits shown as one.
struct column
{
    int   lsb       = 0;
    int   msb       = 0;
    bool  collapsed = false;   // a collapsed segment
    bool  unused    = false;   // an abbreviated run nothing uses
    int   segment   = -1;      // index into the layout's segments
    float x         = 0.0f;    // from the cells' left edge
    float w         = 0.0f;
};

// band
//   struct: one segment as the heading shows it.
struct band
{
    int         lsb       = 0;
    int         msb       = 0;
    int         user      = -1;   // the sheet's segment, or -1: automatic
    bool        collapsed = false;
    std::string name;
    rgba        colour    = 0;
};

// row_kind_shown
//   enum: what a grid row is: the probe, a row of the sheet's, the usage
// row, the add row -- and the "Add child..." field under the rows of the
// group or range a name typed there goes into.
enum class shown
{
    probe,
    sheet,
    usage,
    add,
    child
};

// grid_row
//   struct: one displayed row. A probe or usage row has the special row it
// shows; among the rows, how deep it is indented, and -- a usage row -- the
// group or range whose rows it counts, 0 for the sheet's.
struct grid_row
{
    shown               kind     = shown::sheet;
    const resolved_row* resolved = nullptr;
    const row*          source   = nullptr;
    const special_row*  special  = nullptr;
    int                 depth    = 0;
    int                 scope    = 0;
};

// stroke
//   struct: a press-and-drag across one row's cells, from press to release.
struct stroke
{
    bool active = false;
    int  row    = 0;
    int  row_to = 0;
    bool rectangle = false;
    int  from   = -1;
    int  to     = -1;
    bool set    = true;
};

// resize
//   struct: a drag of the name or value column's right edge.
struct resize
{
    bool        active = false;
    std::string column;
};

// header_drag
//   Reorder the pinned Symbols and Value columns by dragging a heading.
struct header_drag
{
    bool active = false;
    bool names = false;
    float start_x = 0.0f;
};

// the_header_drag / the_stroke / the_resize / the_widening /
// the_band_click / the_draft / the_rest / the_sighting
//   the grid's state between frames, of the document whose grid is being
// drawn: each document's grid has its own, so a drag in one never reaches
// another drawn after it in the same frame.
header_drag&  the_header_drag();

// widening
//   struct: a drag of a boundary in the ruler, which sizes every bit's cell
// at once: the boundary, the cells' width when the drag began, where the
// mouse was then, and how many cells lie left of the boundary -- the ones
// the mouse's movement is shared between.
struct widening
{
    bool  active   = false;
    int   boundary = -1;
    float width    = 0.0f;   // the sheet's, unscaled
    float from     = 0.0f;
    int   cells    = 1;
    float last     = 0.0f;   // the width last asked for
};

// band_click
//   struct: the band the last press in the heading collapsed or expanded,
// so a double-click can put it back as it was before renaming it. An
// automatic band the press collapsed became the sheet's segment `created`.
struct band_click
{
    bool   valid   = false;
    double time    = 0.0;
    long   user    = -1;
    long   created = -1;
    int    lsb     = 0;
    int    msb     = 0;
};

// draft
//   struct: a rename being typed in the grid: whose, and the text so far.
struct draft
{
    int         row     = 0;
    int         segment = -1;
    std::string text;
    bool        focus   = false;   // the field is to take the keyboard
    bool        live    = false;   // the field has had the keyboard
};

// spot
//   struct: what lies under a point of the grid.
struct spot
{
    bool header   = false;   // in the heading
    bool band     = false;   // in the heading's band strip
    bool ruler    = false;   // in the heading's ruler
    bool names    = false;   // in the pinned name and value columns
    bool name     = false;   // in the name column itself
    int  row      = -1;      // the row, or -1
    int  column   = -1;      // the column, or -1
    int  edge     = 0;       // 1: the name column's edge; 2: the value's
    int  boundary = -1;      // a ruler boundary: the column left of it
    int  by       = -1;      // a button over the name column: 0 symbol
                             // name, 1 name, 2 value
};

// name_button
//   struct: one of the buttons over the name column, as a frame lays it
// out: where it lies, and what it says.
struct name_button
{
    ImVec2      min;
    ImVec2      max;
    const char* label = "";
};

// EDGE_REACH
//   constant: how near an edge or a boundary the mouse must be to drag it,
// at a scale of one.
const float EDGE_REACH = 4.0f;

// SQUARE_MOST
//   constant: the widest a square cell grows to fill the grid's room, at a
// scale of one: the most the cell settings allow. Past it a row would be
// taller than any cell height a sheet can ask for, and the room is left.
const float SQUARE_MOST = 56.0f;

// LABEL_CHIP
//   constant: how opaque the chip under a range's name is, over its cells.
const float LABEL_CHIP = 0.85f;

// PLAIN_SET
//   constant: how opaque a set bit is drawn with no colour -- a row with no
// fill, or a cell whose colour the display rules hide: the theme's text,
// faintly, so a set bit always shows.
const float PLAIN_SET = 0.30f;

// NAMES_BY / NAMES_BY_TIPS
//   constants: the labels of the buttons over the symbols column, which
// choose what it lists each row by -- its symbol name, its own name, or its
// value -- longest first, a column too narrow for one set taking the next;
// and what the first two buttons' tooltips say. The value button says, in
// the two longer sets, how values are written while the column lists them.
const char* const NAMES_BY[3][3] = {
    { "symbol name", "name", "value" },
    { "symbol",      "name", "value" },
    { "S",           "N",    "V" } };
const char* const NAMES_BY_TIPS[2] = {
    "list each row by its symbol name: the macro the header defines",
    "list each row by its own name, as the symbols panel does" };

// FORM_LABELS / FORM_WAYS
//   constants: what the value button says while the symbols column writes
// SORT_WORDS
//   constant: each sort key, as the heading and its tooltip write it.
const char* const SORT_WORDS[SORT_KEY_COUNT] = {
    "the sheet", "symbol name", "name", "short name", "value", "lowest bit",
    "width", "kind", "note" };

// values in each form, as value_form numbers them -- "value" for the way
// the header writes them -- and how its tooltip names each form.
const char* const FORM_LABELS[VALUE_FORM_COUNT] = {
    "value", "hex", "dec", "bin", "shift", "bits" };
const char* const FORM_WAYS[VALUE_FORM_COUNT] = {
    "as the header writes them",
    "in hex",
    "in decimal",
    "in binary",
    "as a run shifted into place, 0x3 << 4",
    "as the bits they set, 5:4" };

// RUNS_MOST
//   constant: how many runs of bits the symbols column names, by value as
// bits, before it says how many more there are.
const int RUNS_MOST = 4;

// BUTTON_TEXT / BUTTON_PAD / BUTTON_RISE / BUTTON_FADE
//   constants: the size the symbols column's buttons write at, against the
// interface's font -- the ruler's size; the room either side of a label,
// and above and below it, at a scale of one; and how opaque the words of a
// button not chosen are until the mouse is on it.
const float BUTTON_TEXT = 0.82f;
const float BUTTON_PAD  = 5.0f;
const float BUTTON_RISE = 2.0f;
const float BUTTON_FADE = 0.70f;

// ROW_DRAG
//   constant: what a row dragged by its name carries, as the tree's rows
// do: its id, as a long.
const char* const ROW_DRAG = "bitwisedom.row";

// SPECIAL_DRAG
//   constant: what a probe or usage row dragged by its name carries: its
// id among the special rows, as a long.
const char* const SPECIAL_DRAG = "bitwisedom.special";

// tally
//   struct: what a usage row counts: how many symbols allocate each bit,
// and how many bits are allocated at all.
struct tally
{
    std::vector<int> usage;
    int              allocated = 0;
};

// layout
//   struct: the grid's geometry for one frame.
struct layout
{
    std::vector<column>   columns;
    std::vector<band>     bands;
    std::vector<grid_row> rows;
    float name_w   = 0.0f;
    float value_w  = 0.0f;
    bool values_first = false;
    float cells_w  = 0.0f;
    float cell_w   = 0.0f;   // a bit's cell, as it is drawn
    float room     = 0.0f;   // the width the grid was laid out to fill
    float band_h   = 0.0f;
    float ruler_h  = 0.0f;
    float header_h = 0.0f;
    float row_h    = 0.0f;

    // the special rows pinned above every row, and below: the first are the
    // rows' first, the last their last; and the add row's index, where the
    // rows of the tree end
    std::size_t pinned_top    = 0;
    std::size_t pinned_bottom = 0;
    std::size_t tree_end      = 0;

    // what the usage rows among the rows count, by the group or range whose
    // rows they count
    std::map<int, tally> subtotals;
};

stroke&       the_stroke();
resize&       the_resize();
widening&     the_widening();
band_click&   the_band_click();
draft&        the_draft();

// rest
//   struct: what the mouse rests on in the grid's pinned columns or its
// heading, and since when: their tooltips wait for a rest, as ImGui's own
// do, where a cell's shows at once.
struct rest
{
    long   key   = -1;
    double since = 0.0;
};

rest&         the_rest();

// rested
//   whether the mouse has rested on what _key names long enough for its
// tooltip; -1 is nothing, and starts the wait again.
bool
rested(
    long _key
)
{
    rest&        held = the_rest();
    const double now  = ImGui::GetTime();

    if (held.key != _key)
    {
        held.key   = _key;
        held.since = now;
    }

    return ( (_key >= 0) &&
             (now - held.since >= static_cast<double>(
                                      ImGui::GetStyle().HoverDelayNormal)) );
}

// cells_drawn
//   struct: how a row's cells in view were drawn: how many wrote a digit,
// how many were filled in the row's colour, and how many set ones were
// drawn plain.
struct cells_drawn
{
    int digits   = 0;
    int coloured = 0;
    int plain    = 0;
};

// sighting
//   struct: where the last frame drew the grid's parts, for grid_spot.
struct sighting
{
    bool                drawn    = false;
    ImVec2              origin;
    ImVec2              view_min;
    float               name_w   = 0.0f;
    float               value_w  = 0.0f;
    bool                values_first = false;
    float               band_h   = 0.0f;
    float               ruler_h  = 0.0f;
    float               header_h = 0.0f;
    float               row_h    = 0.0f;
    float               end      = 0.0f;   // where the columns end
    float               room     = 0.0f;   // the width they were laid out to
    std::vector<column> columns;
    std::vector<int>    ids;     // each row's; 0 for the probe and usage
    std::vector<int>    specials;   // each special row's id; 0 for others
    std::vector<float>  tops;    // where each row's top was
    std::vector<float>  names;   // where each row's name begins
    int                 add      = -1;     // the add row's index
    int                 child    = -1;     // the "Add child..." row's
    float               add_x    = 0.0f;   // the middle of its label
    std::vector<ImVec2> buttons; // the middle of each name column button
    std::vector<std::string> labels;   // what the name column wrote for
                                       // each row it drew, before fitting
    std::vector<float>       ends;     // ... and where it ended
    std::vector<std::string> notes;    // what a row with no bits wrote
                                       // over its cells
    std::vector<cells_drawn> painted;  // how each row's cells were drawn
};

// grid_state
//   struct: one document's grid between frames.
struct grid_state
{
    int         named = 0;   // the row a press on a name began on: what a
                             // drag from it carries
    int         special = 0; // ... or the special row's
    header_drag headings;
    stroke      drag;
    resize      sizing;
    widening    widen;
    band_click  band;
    draft       typed;
    rest        resting;
    sighting    seen;
};

// states / drawing / drawn_last
//   every document's grid state, by the document's id; the document whose
// grid is being drawn; and the one drawn last, which grid_spot reads.
std::map<int, grid_state>&
states()
{
    static std::map<int, grid_state> held;

    return held;
}

int&
drawing()
{
    static int id = 0;

    return id;
}

int&
drawn_last()
{
    static int id = 0;

    return id;
}

header_drag&
the_header_drag()
{
    return states()[drawing()].headings;
}

stroke&
the_stroke()
{
    return states()[drawing()].drag;
}

resize&
the_resize()
{
    return states()[drawing()].sizing;
}

widening&
the_widening()
{
    return states()[drawing()].widen;
}

band_click&
the_band_click()
{
    return states()[drawing()].band;
}

draft&
the_draft()
{
    return states()[drawing()].typed;
}

rest&
the_rest()
{
    return states()[drawing()].resting;
}

sighting&
the_sighting()
{
    return states()[drawing()].seen;
}

int&
the_named()
{
    return states()[drawing()].named;
}

int&
the_special()
{
    return states()[drawing()].special;
}

/*
count_within
  What a usage row in a group counts: the bit and range symbols with a value
among the group's rows, at any depth -- the rows after it in the
resolution's depth-first order, until one no deeper than it.
*/
tally
count_within(
    const resolution& _res,
    int               _scope,
    int               _width
)
{
    tally out;

    out.usage.assign(static_cast<std::size_t>(std::max(_width, 0)), 0);

    const auto found = _res.index.find(_scope);

    if (found == _res.index.end())
    {
        return out;
    }

    const std::size_t from  = found->second;
    const int         depth = _res.rows[from].depth;
    wide              any;

    for (std::size_t i = from + 1;
         ( (i < _res.rows.size()) &&
           (_res.rows[i].depth > depth) );
         ++i)
    {
        const resolved_row& each = _res.rows[i];

        if ( (each.kind != row_kind::symbol) ||
             (!each.has_value)               ||
             ( (each.def != def_kind::bit) &&
               (each.def != def_kind::range) ) )
        {
            continue;
        }

        any |= each.bits;

        for (int b = each.bits.next(0);
             (b >= 0) && (b < _width);
             b = each.bits.next(b + 1))
        {
            ++out.usage[static_cast<std::size_t>(b)];
        }
    }

    any.trim(_width);
    out.allocated = static_cast<int>(any.count());

    return out;
}

// any_in / count_in
//   whether a value has a bit set in [_lsb, _msb], and how many: what a
// column shows of a row, in a sheet of any width.
bool
any_in(
    const wide& _value,
    int         _lsb,
    int         _msb
)
{
    const int first = _value.next(_lsb);

    return ( (first >= 0) &&
             (first <= _msb) );
}

int
count_in(
    const wide& _value,
    int         _lsb,
    int         _msb
)
{
    int count = 0;

    for (int b = _value.next(_lsb);
         (b >= 0) && (b <= _msb);
         b = _value.next(b + 1))
    {
        ++count;
    }

    return count;
}

// bit_label
//   a bit's number as the sheet's numbering names it.
int
bit_label(
    const sheet& _sheet,
    int          _bit
)
{
    return (_sheet.bits == numbering::lsb0) ? _bit
                                            : (_sheet.width - 1 - _bit);
}

// byte_label
//   the byte a bit is in, as the sheet's byte numbering names it.
int
byte_label(
    const sheet& _sheet,
    int          _bit
)
{
    return (_sheet.bytes == byte_order::little)
               ? (_bit / 8)
               : ((_sheet.width - 1 - _bit) / 8);
}

// kind_colour
//   the colour a definition kind is drawn in, in the "kind" colour mode.
rgba
kind_colour(
    def_kind _kind
)
{
    switch (_kind)
    {
        case def_kind::bit:         return 0x4C8DFFFFu;
        case def_kind::range:       return 0x2EC4B6FFu;
        case def_kind::field_value: return 0xF2A541FFu;
        case def_kind::literal:     return 0xB388EBFFu;
        case def_kind::expression:  return 0xF25F5CFFu;
        case def_kind::macro:       return 0x8D99AEFFu;
    }

    return 0x808080FFu;
}

// set_colour
//   the colour a row's set bits are drawn in.
rgba
set_colour(
    const model&        _model,
    const resolved_row& _row
)
{
    const colour_settings& colours = _model.shown.colours;
    const row* source = find_row(_model.doc, _row.id);
    if ((source != nullptr) && (source->transparent))
    {
        return 0;
    }

    switch (colours.mode)
    {
        case colour_mode::group:
        {
            const resolved_row* parent = _model.res.find(_row.parent);

            return (parent != nullptr) ? parent->colour : _row.colour;
        }

        case colour_mode::kind:
            return kind_colour(_row.def);

        case colour_mode::accent:
            return colours.accent;

        case colour_mode::row:
            break;
    }

    return _row.colour;
}

// initials
//   the first letter of each underscore-separated part: PARITY_EVEN, PE.
std::string
initials(
    const std::string& _name
)
{
    std::string out;
    bool        start = true;

    for (char each : _name)
    {
        if (each == '_')
        {
            start = true;

            continue;
        }

        if (start)
        {
            out += each;
            start = false;
        }
    }

    return out;
}

// fit
//   the longest of a name's forms that fits a width: the name, its short
// form, its initials, or as much of it as fits with ".." after. A wide
// sheet's values run to thousands of digits, so the cut is found by
// halving: a longer start is never the narrower.
std::string
fit(
    const std::string& _name,
    const std::string& _short,
    float              _width
)
{
    if (ImGui::CalcTextSize(_name.c_str()).x <= _width)
    {
        return _name;
    }

    if ( (!_short.empty()) &&
         (ImGui::CalcTextSize(_short.c_str()).x <= _width) )
    {
        return _short;
    }

    const std::string first = initials(_name);

    if ( (first.size() > 1) &&
         (first.size() < _name.size()) &&
         (ImGui::CalcTextSize(first.c_str()).x <= _width) &&
         (_width < ImGui::CalcTextSize("MMMMMM").x) )
    {
        return first;
    }

    // cut
    //   the name's first _length characters, and "..".
    auto cut = [&_name](std::size_t _length)
    {
        return _name.substr(0, _length) + "..";
    };

    if (ImGui::CalcTextSize("..").x > _width)
    {
        return std::string();
    }

    // the longest start that fits lies in [fits, too_long)
    std::size_t fits     = 0;
    std::size_t too_long = _name.size();

    while (too_long - fits > 1)
    {
        const std::size_t middle = fits + (too_long - fits) / 2;

        if (ImGui::CalcTextSize(cut(middle).c_str()).x <= _width)
        {
            fits = middle;
        }
        else
        {
            too_long = middle;
        }
    }

    return cut(fits);
}

// fit_tail
//   as much of a value's end as fits a width, with ".." before it: a value
// lined up at the right keeps its low digits, where values differ most.
std::string
fit_tail(
    const std::string& _value,
    float              _width
)
{
    if (ImGui::CalcTextSize(_value.c_str()).x <= _width)
    {
        return _value;
    }

    if (ImGui::CalcTextSize("..").x > _width)
    {
        return std::string();
    }

    // cut
    //   the value's last _length characters, after "..".
    auto cut = [&_value](std::size_t _length)
    {
        return ".." + _value.substr(_value.size() - _length);
    };

    // the longest end that fits lies in [fits, too_long)
    std::size_t fits     = 0;
    std::size_t too_long = _value.size();

    while (too_long - fits > 1)
    {
        const std::size_t middle = fits + (too_long - fits) / 2;

        if (ImGui::CalcTextSize(cut(middle).c_str()).x <= _width)
        {
            fits = middle;
        }
        else
        {
            too_long = middle;
        }
    }

    return cut(fits);
}

// shown_value
//   a value as the value column writes it: in its row's radix, with the
// radix's C prefix.
std::string
shown_value(
    u64   _value,
    radix _radix,
    int   _width
)
{
    switch (_radix)
    {
        case radix::dec:
            return format_dec(_value);

        case radix::oct:
            return (_value == 0) ? "0"
                                 : "0" + group_digits(format_oct(_value), 3);

        case radix::bin:
            return "0b" + format_bin(_value, _width, 8, ' ');

        default:
            break;
    }

    return "0x" + group_digits(
        format_hex(_value, (_width + 3) / 4, true), 2);
}

// shown_wide
//   a value as the value column writes it in a sheet of any width: as the
// narrow form where it fits an integer, else as its runs -- a wide sheet's
// hex is mostly zeros, and says less than where its bits are.
std::string
shown_wide(
    const wide& _value,
    radix       _radix,
    int         _width
)
{
    if (!is_wide(_width))
    {
        return shown_value(_value.low(), _radix, _width);
    }

    return runs_text(_value, 2);
}

// form_fits
//   whether a sheet _width bits wide writes values in _form: decimal and
// binary only within an integer's width -- a wider sheet writes them in
// hex.
bool
form_fits(
    value_form _form,
    int        _width
)
{
    return ( (!is_wide(_width))              ||
             ( (_form != value_form::dec) &&
               (_form != value_form::bin) ) );
}

// next_form
//   the form a press on the value button takes after _form: the next one,
// round again after the last, past any a sheet _width bits wide does not
// write.
value_form
next_form(
    value_form _form,
    int        _width
)
{
    int at = static_cast<int>(_form);

    for (int i = 0; i < VALUE_FORM_COUNT; ++i)
    {
        at = (at + 1) % VALUE_FORM_COUNT;

        if (form_fits(static_cast<value_form>(at), _width))
        {
            return static_cast<value_form>(at);
        }
    }

    return value_form::written;
}

// draw_line
//   one grid line, solid, dashed or dotted, cut to the stretch of it the
// clip rectangle shows: a line across a sheet of thousands of bits runs a
// million pixels past the window, and a GPU given such a line may draw
// stray pixels where it should draw none.
void
draw_line(
    ImDrawList*      _list,
    ImVec2           _a,
    ImVec2           _b,
    const line_spec& _spec
)
{
    const ImU32 colour = to_u32(_spec.colour);
    const float dx     = _b.x - _a.x;
    const float dy     = _b.y - _a.y;
    const float length = std::sqrt(dx * dx + dy * dy);

    if (length <= 0.0f)
    {
        return;
    }

    const float ux = dx / length;
    const float uy = dy / length;

    // the stretch of the line the clip rectangle shows, as distances along
    // it: a line down a sheet of a thousand rows is thousands of dashes, of
    // which a window shows a few dozen
    const float  margin = _spec.thickness + 1.0f;
    const ImVec2 low(_list->GetClipRectMin().x - margin,
                     _list->GetClipRectMin().y - margin);
    const ImVec2 high(_list->GetClipRectMax().x + margin,
                      _list->GetClipRectMax().y + margin);
    float        first  = 0.0f;
    float        last   = length;

    // narrow
    //   the stretch, narrowed to where one coordinate is within its span;
    // false when nothing of the line is left.
    auto narrow = [&first, &last](float _start,
                                  float _step,
                                  float _min,
                                  float _max)
    {
        // a line that does not move in this coordinate is in or out whole
        if (std::fabs(_step) < 1e-6f)
        {
            return ( (_start >= _min) &&
                     (_start <= _max) );
        }

        const float enter = (_min - _start) / _step;
        const float leave = (_max - _start) / _step;

        first = std::max(first, std::min(enter, leave));
        last  = std::min(last, std::max(enter, leave));

        return (first < last);
    };

    if ( (!narrow(_a.x, ux, low.x, high.x)) ||
         (!narrow(_a.y, uy, low.y, high.y)) )
    {
        return;
    }

    if (_spec.style == line_style::solid)
    {
        _list->AddLine(ImVec2(_a.x + ux * first, _a.y + uy * first),
                       ImVec2(_a.x + ux * last, _a.y + uy * last),
                       colour,
                       _spec.thickness);

        return;
    }

    const float on     = (_spec.style == line_style::dashed)
                             ? 5.0f
                             : std::max(1.0f, _spec.thickness);
    const float gap    = (_spec.style == line_style::dashed) ? 4.0f : 3.0f;
    const float period = on + gap;

    // a dash or dot at a time along the line, from the first one shown;
    // the dashes keep their places, counted from the line's start, so they
    // do not crawl as it scrolls
    for (float t = std::floor(first / period) * period;
         t < last;
         t += period)
    {
        const float end = std::min(length, t + on);

        _list->AddLine(ImVec2(_a.x + ux * t, _a.y + uy * t),
                       ImVec2(_a.x + ux * end, _a.y + uy * end),
                       colour,
                       _spec.thickness);
    }

    return;
}

// band_index
//   struct: the bands made so far, by the sheet's segment and by automatic
// block, so that each of thousands of bits finds its band at once.
struct band_index
{
    std::vector<int> user;    // the band of each of the sheet's segments
    std::vector<int> block;   // the band of each automatic block
};

// band_for
//   the band a bit lies in: the first of the sheet's segments that holds
// it, else the automatic one, else none (-1).
int
band_for(
    const sheet&       _sheet,
    std::vector<band>& _bands,
    band_index&        _index,
    int                _bit
)
{
    for (std::size_t i = 0; i < _sheet.segments.size(); ++i)
    {
        const column_segment& each = _sheet.segments[i];
        const int             low  = std::min(each.lsb, each.msb);
        const int             high = std::max(each.lsb, each.msb);

        if ( (_bit < low) ||
             (_bit > high) )
        {
            continue;
        }

        if (_index.user.size() < _sheet.segments.size())
        {
            _index.user.assign(_sheet.segments.size(), -1);
        }

        if (_index.user[i] >= 0)
        {
            return _index.user[i];
        }

        band made;
        made.lsb       = std::max(0, low);
        made.msb       = std::min(_sheet.width - 1, high);
        made.user      = static_cast<int>(i);
        made.collapsed = each.collapsed;
        made.name      = each.name;
        made.colour    = each.colour;
        _bands.push_back(made);
        _index.user[i] = static_cast<int>(_bands.size()) - 1;

        return _index.user[i];
    }

    const int every = _sheet.grid.auto_segments;

    if (every <= 0)
    {
        return -1;
    }

    const int        low   = (_bit / every) * every;
    const int        high  = std::min(_sheet.width - 1, low + every - 1);
    const std::size_t block = static_cast<std::size_t>(_bit / every);

    if (_index.block.size() <= block)
    {
        _index.block.resize(block + 1, -1);
    }

    if (_index.block[block] >= 0)
    {
        return _index.block[block];
    }

    _index.block[block] = static_cast<int>(_bands.size());

    band made;
    made.lsb  = low;
    made.msb  = high;
    made.name = (every == 8)
                    ? "byte " + format_dec(static_cast<u64>(
                                    byte_label(_sheet, low)))
                    : format_dec(static_cast<u64>(bit_label(_sheet, high))) +
                          ":" +
                          format_dec(static_cast<u64>(bit_label(_sheet,
                                                                low)));
    _bands.push_back(made);

    return static_cast<int>(_bands.size()) - 1;
}

/*
adding_into
  The group or range a name typed into the grid's "Add child..." field goes
into: the row selected first, when it holds rows and is open; else the
group or range it is in -- not the sheet's root, whose rows the add row
adds to. 0 when nothing is selected.
*/
int
adding_into(
    const model& _model
)
{
    if (_model.ui.selection.empty())
    {
        return 0;
    }

    const int           first  = _model.ui.selection.front();
    const row*          chosen = find_row(_model.doc, first);
    const resolved_row* shown  = _model.res.find(first);

    if ( (chosen == nullptr) ||
         (shown == nullptr) )
    {
        return 0;
    }

    if (can_contain(*chosen))
    {
        return chosen->collapsed ? 0 : chosen->id;
    }

    return (shown->parent == _model.doc.root.id) ? 0 : shown->parent;
}

// lay_out
//   the frame's columns and rows, filling _room as the sheet says.
layout
lay_out(
    const model& _model,
    float        _room
)
{
    const sheet&         doc  = _model.doc;
    const grid_settings& grid = doc.grid;
    layout               out;
    const float          line = ImGui::GetTextLineHeight();

    const float scale  = ui_scale();
    const float lowest = line + 4.0f;   // a row's least: a line, and room
    const bool  square = ( (grid.square_cells) &&
                           (grid.show_bits) );

    // a bit's cell before the room to spare is shared: as wide as the sheet
    // says, whatever the rows -- proportional rows stop at the shortest a
    // line of digits fits in, and no narrower cell makes them shorter
    const float ratio  = std::clamp(grid.cell_ratio, 0.4f, 1.5f);
    const float plain  = grid.cell_width * scale;

    out.row_h   = std::max(grid.cell_height * scale, lowest);
    out.name_w  = grid.show_names ? grid.name_width * scale : 0.0f;
    out.value_w = grid.show_values ? grid.value_width * scale : 0.0f;
    out.values_first = grid.values_first && grid.show_values && grid.show_names;
    out.room    = _room;

    // The symbol tree is followed by an add row, then optional utilities
    // anchored to the bottom of the grid (not above the symbols).
    // the bits something shown uses -- worked out only when unused runs
    // are abbreviated, which is all it is for
    wide            used = grid.show_probe ? doc.probe : wide();
    const row_index rows(doc);

    // the rows in the order they are shown: the sheet's, or sorted
    const std::vector<const resolved_row*> all = shown_rows(_model);

    // the group or range a name typed into the grid goes into, and the row
    // its field follows: the last of its rows in view, or itself
    const int           holder  = adding_into(_model);
    const resolved_row* holding = nullptr;
    int                 after   = 0;

    if (holder != 0)
    {
        for (std::size_t i = 0; i < all.size(); ++i)
        {
            if (all[i]->id != holder)
            {
                continue;
            }

            if (all[i]->visible)
            {
                holding = all[i];
                after   = all[i]->id;

                for (std::size_t j = i + 1;
                     ( (j < all.size()) &&
                       (all[j]->depth > all[i]->depth) );
                     ++j)
                {
                    if (all[j]->visible)
                    {
                        after = all[j]->id;
                    }
                }
            }

            break;
        }
    }

    // the special rows shown -- of the kinds shown -- in their order; those
    // among the rows by where they go: before a row, or at the end of a
    // group's or range's rows, or of the sheet's, 0
    std::vector<const special_row*>      pinned_top;
    std::vector<const special_row*>      pinned_bottom;
    std::multimap<int, const special_row*> before_row;
    std::multimap<int, const special_row*> ending;

    for (const special_row& each : grid.specials)
    {
        const bool on = (each.kind == special_kind::probe) ? grid.show_probe
                                                           : grid.show_usage;

        if (!on)
        {
            continue;
        }

        if (each.place == special_place::top)
        {
            pinned_top.push_back(&each);
        }
        else if (each.place == special_place::bottom)
        {
            pinned_bottom.push_back(&each);
        }
        else if ( (each.before != 0) &&
                  (_model.res.find(each.before) != nullptr) )
        {
            before_row.emplace(each.before, &each);
        }
        else
        {
            const row* container = (each.parent != 0)
                                       ? rows.find(each.parent)
                                       : nullptr;

            ending.emplace( ( (container != nullptr) &&
                              (can_contain(*container)) ) ? each.parent : 0,
                            &each);
        }
    }

    // special
    //   a special row's grid row: indented to the rows it is among, and a
    // usage row counting its scope's.
    auto special = [&out, &_model, &doc](const special_row* _each,
                                         int                _depth,
                                         int                _scope)
    {
        grid_row made;

        made.kind    = (_each->kind == special_kind::probe) ? shown::probe
                                                            : shown::usage;
        made.special = _each;
        made.depth   = _depth;
        made.scope   = _scope;
        out.rows.push_back(made);

        if ( (_scope != 0) &&
             (_each->kind == special_kind::usage) &&
             (out.subtotals.find(_scope) == out.subtotals.end()) )
        {
            out.subtotals[_scope] = count_within(_model.res, _scope,
                                                 doc.width);
        }

        return;
    };

    for (const special_row* each : pinned_top)
    {
        special(each, 0, 0);
    }

    out.pinned_top = out.rows.size();

    // the rows whose rows are being listed, innermost last: a group's or a
    // range's rows end at the next row no deeper than it, and the special
    // rows at their end -- in an open one -- follow them
    struct listing
    {
        int  id    = 0;
        int  depth = 0;
        bool open  = false;
    };

    std::vector<listing> listed;

    auto close_to = [&listed, &ending, &special](int _depth)
    {
        while ( (!listed.empty()) &&
                (listed.back().depth >= _depth) )
        {
            const listing closing = listed.back();

            listed.pop_back();

            if (!closing.open)
            {
                continue;
            }

            const auto range = ending.equal_range(closing.id);

            for (auto it = range.first; it != range.second; ++it)
            {
                special(it->second, closing.depth + 1, closing.id);
            }
        }

        return;
    };

    for (const resolved_row* shown_row : all)
    {
        const resolved_row& each = *shown_row;

        if (!each.visible)
        {
            continue;
        }

        close_to(each.depth);

        // the special rows that come before it, among its siblings
        const auto ahead = before_row.equal_range(each.id);

        for (auto it = ahead.first; it != ahead.second; ++it)
        {
            special(it->second, each.depth, each.parent);
        }

        const row* source = rows.find(each.id);

        out.rows.push_back(grid_row{ shown::sheet,
                                     &each,
                                     source });
        listed.push_back(listing{ each.id,
                                  each.depth,
                                  ( (source != nullptr)     &&
                                    (can_contain(*source))  &&
                                    (!source->collapsed) ) });

        if ( (holding != nullptr) &&
             (each.id == after) )
        {
            out.rows.push_back(grid_row{ shown::child,
                                         holding,
                                         rows.find(holding->id) });
        }

        if (!grid.abbreviate_unused)
        {
            continue;
        }

        if (each.has_value)
        {
            used |= each.bits;
        }

        // a value's whole field counts as used
        if (each.span_width > 0)
        {
            used |= wide::run(each.span_lsb, each.span_width);
        }
    }

    // the groups still open end, and the sheet's own rows
    close_to(-1);

    {
        const auto range = ending.equal_range(0);

        for (auto it = range.first; it != range.second; ++it)
        {
            special(it->second, 0, 0);
        }
    }

    out.tree_end = out.rows.size();
    out.rows.push_back(grid_row{ shown::add, nullptr, nullptr });

    for (const special_row* each : pinned_bottom)
    {
        special(each, 0, 0);
    }

    out.pinned_bottom = out.rows.size() - out.tree_end - 1;

    // the columns, in display order
    std::vector<column> raw;
    std::vector<int>    owner(static_cast<std::size_t>(doc.width), -1);
    band_index          index;

    for (int i = 0; i < doc.width; ++i)
    {
        const int bit = doc.msb_left ? (doc.width - 1 - i) : i;

        owner[static_cast<std::size_t>(bit)] =
            band_for(doc, out.bands, index, bit);
    }

    for (int i = 0; i < doc.width; ++i)
    {
        const int bit   = doc.msb_left ? (doc.width - 1 - i) : i;
        const int which = owner[static_cast<std::size_t>(bit)];

        // a collapsed band is one column, met at its first bit
        if ( (which >= 0) &&
             (out.bands[static_cast<std::size_t>(which)].collapsed) )
        {
            if ( (!raw.empty()) &&
                 (raw.back().segment == which) )
            {
                raw.back().lsb = std::min(raw.back().lsb, bit);
                raw.back().msb = std::max(raw.back().msb, bit);

                continue;
            }

            column made;
            made.lsb       = bit;
            made.msb       = bit;
            made.collapsed = true;
            made.segment   = which;
            raw.push_back(made);

            continue;
        }

        column made;
        made.lsb     = bit;
        made.msb     = bit;
        made.segment = which;
        raw.push_back(made);
    }

    // runs nothing uses, outside the sheet's own segments, abbreviated
    std::vector<column>& columns = out.columns;

    for (std::size_t i = 0; i < raw.size(); )
    {
        const column& first = raw[i];
        const bool    open  =
            ( (grid.abbreviate_unused) &&
              (!first.collapsed)       &&
              ( (first.segment < 0) ||
                (out.bands[static_cast<std::size_t>(first.segment)].user <
                 0) ) &&
              (!used.test(first.lsb)) );

        if (!open)
        {
            columns.push_back(first);
            ++i;

            continue;
        }

        std::size_t end = i;

        while ( (end < raw.size())                                  &&
                (!raw[end].collapsed)                               &&
                (raw[end].segment == first.segment)                 &&
                (!used.test(raw[end].lsb)) )
        {
            ++end;
        }

        if (static_cast<int>(end - i) >= grid.unused_run)
        {
            column made = first;
            made.unused = true;

            for (std::size_t k = i; k < end; ++k)
            {
                made.lsb = std::min(made.lsb, raw[k].lsb);
                made.msb = std::max(made.msb, raw[k].msb);
            }

            columns.push_back(made);
        }
        else
        {
            for (std::size_t k = i; k < end; ++k)
            {
                columns.push_back(raw[k]);
            }
        }

        i = end;
    }

    if (!grid.show_bits)
    {
        columns.clear();
    }

    // widths: a bit is a cell; a run is wide enough for its label
    for (column& each : columns)
    {
        if ( (each.collapsed) ||
             (each.unused) )
        {
            // wide enough for the ruler's "31:28" and for the digest
            const int         digits = (each.msb - each.lsb + 4) / 4;
            const std::string widest(static_cast<std::size_t>(digits), 'F');
            const std::string span   =
                format_dec(static_cast<u64>(each.msb)) + ":" +
                format_dec(static_cast<u64>(each.lsb));
            const float       ruler  =
                ImGui::CalcTextSize(span.c_str()).x * 0.82f + 10.0f;

            each.w = std::max({ grid.cell_width * scale * 1.6f,
                                ImGui::CalcTextSize(widest.c_str()).x + 10.0f,
                                ruler });
        }
        else
        {
            each.w = plain;
        }
    }

    // the room to spare: the cells take it in proportion, to whole pixels,
    // the last ending at the grid's edge -- square cells, until they are as
    // wide as one may be; or the name column takes it all
    float natural = 0.0f;

    for (const column& each : columns)
    {
        natural += each.w;
    }

    const float spare = std::floor(_room - out.name_w - out.value_w) -
                        natural;

    out.cell_w = plain;

    if ( (spare >= 1.0f)   &&
         (natural > 0.0f) )
    {
        if (grid.fill == grid_fill::cells)
        {
            const float most   = std::max(plain, SQUARE_MOST * scale);
            const float widest = std::max(natural,
                                          std::floor(natural * most / plain));
            const float target = square ? std::min(natural + spare, widest)
                                        : natural + spare;
            float       sum    = 0.0f;
            float       left   = 0.0f;

            for (std::size_t i = 0; i < columns.size(); ++i)
            {
                sum += columns[i].w;

                const float right = (i + 1 == columns.size())
                                        ? target
                                        : std::floor(sum * target / natural);

                columns[i].w = right - left;
                left         = right;
            }

            out.cell_w = std::floor(out.cell_w * target / natural);
        }
        else if (grid.fill == grid_fill::names)
        {
            out.name_w += spare;
        }
    }

    float x = 0.0f;

    for (column& each : columns)
    {
        each.x = x;
        x     += each.w;
    }

    out.cells_w  = x;

    // proportional cells: every row as tall as a bit's cell is wide, times
    // the sheet's share -- 1 for square -- the cell height aside
    if ( (square) &&
         (!columns.empty()) )
    {
        out.row_h = std::max(std::floor(out.cell_w * ratio), lowest);
    }

    if (!grid.show_bits && _room > out.name_w + out.value_w)
    {
        if (out.name_w > 0.0f) out.name_w = _room - out.value_w;
        else if (out.value_w > 0.0f) out.value_w = _room;
    }
    // the heading: the band strip -- kept with no bands while the name
    // column is shown, for the buttons over it -- the ruler, and the
    // columns' labels on their own line, below the buttons, ruler or none
    const bool has_buttons = (out.name_w > 0.0f);

    out.band_h   = ( (out.bands.empty()) &&
                     (!has_buttons) ) ? 0.0f
                                      : (line + 6.0f);
    out.ruler_h  = grid.show_indices ? (line + 4.0f) : 0.0f;
    out.header_h = std::max(out.band_h + out.ruler_h,
                            has_buttons ? (out.band_h + line + 4.0f)
                                        : (line + 6.0f));

    return out;
}

// column_at
//   the column under an x measured from the cells' left edge, or -1.
int
column_at(
    const layout& _layout,
    float         _x
)
{
    // the columns lie left to right: the first whose right edge is past _x
    const auto found = std::upper_bound(
        _layout.columns.begin(),
        _layout.columns.end(),
        _x,
        [](float _at, const column& _each)
        {
            return (_at < _each.x + _each.w);
        });

    if (found != _layout.columns.end())
    {
        const column& each = *found;

        if ( (_x >= each.x) &&
             (_x < each.x + each.w) )
        {
            return static_cast<int>(found - _layout.columns.begin());
        }
    }

    return -1;
}

// cell_text
//   what a collapsed column shows of a value.
std::string
cell_text(
    const model&  _model,
    const column& _column,
    const wide&   _value
)
{
    if (_column.unused)
    {
        return "..";
    }

    const int span = _column.msb - _column.lsb + 1;

    switch (_model.doc.grid.collapsed)
    {
        case collapsed_style::hex:
            return format_hex(_value.slice(_column.lsb, span),
                              (span + 3) / 4,
                              true);

        case collapsed_style::dots:
            return any_in(_value, _column.lsb, _column.msb) ? "*" : "";

        case collapsed_style::count:
        {
            const int set = count_in(_value, _column.lsb, _column.msb);

            return (set > 0) ? format_dec(static_cast<u64>(set))
                             : "";
        }

        case collapsed_style::blank:
            break;
    }

    return std::string();
}

// preview_after
//   a row's value after the previewed operation, if it is a target.
bool
preview_after(
    const model& _model,
    int          _id,
    wide&        _after
)
{
    // the preview shows while the operations window is in view
    if ( (!_model.ui.op_preview)    ||
         (!_model.preview_valid)    ||
         (!_model.ui.preview_shown) )
    {
        return false;
    }

    for (const op_row& each : _model.preview.rows)
    {
        if (each.id == _id)
        {
            _after = each.after;

            return true;
        }
    }

    return false;
}

// painter
//   class: the drawing of one frame, with what every part of it needs.
class painter
{
public:
    painter(
        model&        _model,
        imgui_frame&  _frame,
        const layout& _layout
    )
        : m(_model),
          f(_frame),
          l(_layout),
          doc(_model.doc),
          grid(_model.shown.grid),
          colours(_model.shown.colours),
          list(ImGui::GetWindowDrawList())
    {}

    void run();

private:
    model&                 m;
    imgui_frame&           f;
    const layout&          l;
    const sheet&           doc;
    const grid_settings&   grid;
    const colour_settings& colours;
    ImDrawList*            list;
    ImVec2                 origin;     // the content's top-left, scrolled
    ImVec2                 view_min;   // the visible area
    ImVec2                 view_max;
    float                  cells_left = 0.0f;   // pinned edge of the cells
    float                  rows_top   = 0.0f;   // pinned edge of the rows

    // the columns in view, [first, last); and the first that is one bit's
    // cell, which the ruler's boundaries size
    std::size_t            first_column = 0;
    std::size_t            last_column  = 0;
    std::size_t            first_plain  = 0;

    // the buttons over the name column, as this frame lays them out: none
    // when the column is too narrow, or hidden; and, for grid_written, what
    // the column wrote for each row it drew -- before fitting -- and where
    // that ended, and what a row with no bits wrote over its cells
    std::vector<name_button> buttons;
    std::vector<std::string> written;
    std::vector<float>       written_end;
    std::vector<std::string> noted;
    std::vector<cells_drawn> painted;

    // what the mouse is over before the drawing, so the edges can light, the
    // add row's "+ Add new..." brighten, and a button over the name column
    int                    edge_hot     = 0;
    int                    boundary_hot = -1;
    bool                   add_hot      = false;
    int                    button_hot   = -1;

    // the grid's button, as this frame left it
    bool                   over    = false;   // the mouse is over it
    bool                   pressed = false;   // a press began on it
    bool                   twice   = false;   // ... and made a double-click
    bool                   held    = false;   // it is being pressed

    float name_x() const
    {
        return view_min.x + (l.values_first ? l.value_w : 0.0f);
    }

    float value_x() const
    {
        return view_min.x + (l.values_first ? 0.0f : l.name_w);
    }

    float cells_x(float _x) const
    {
        return origin.x + l.name_w + l.value_w + _x;
    }

    // row_y
    //   a row's top. The special rows pinned at the bottom are at the
    // grid's foot while the rows above leave room, and after them when not.
    float row_y(std::size_t _index) const
    {
        float y = origin.y + l.header_h + static_cast<float>(_index) * l.row_h;

        if ( (l.pinned_bottom > 0) &&
             (_index >= l.rows.size() - l.pinned_bottom) )
        {
            const float foot = view_max.y -
                               static_cast<float>(l.rows.size() - _index) *
                                   l.row_h;

            y = std::max(y, foot);
        }

        return y;
    }

    // usage_of
    //   what a usage row counts: its scope's, or the sheet's.
    const std::vector<int>& usage_of(const grid_row& _row) const
    {
        const auto found = l.subtotals.find(_row.scope);

        return ( (_row.scope != 0) &&
                 (found != l.subtotals.end()) ) ? found->second.usage
                                                : m.res.usage;
    }

    int allocated_of(const grid_row& _row) const
    {
        const auto found = l.subtotals.find(_row.scope);

        return ( (_row.scope != 0) &&
                 (found != l.subtotals.end()) )
                   ? found->second.allocated
                   : static_cast<int>(m.res.allocated.count());
    }

    // special_label
    //   what a special row's name column says: "probe", "allocated", or a
    // usage row in a group, "allocated in" the group.
    std::string special_label(const grid_row& _row) const
    {
        if (_row.kind == shown::probe)
        {
            return "probe";
        }

        const resolved_row* scope = (_row.scope != 0) ? m.res.find(_row.scope)
                                                      : nullptr;

        return (scope == nullptr) ? std::string("allocated")
                                  : "allocated in " + (scope->local.empty()
                                                           ? scope->full
                                                           : scope->local);
    }

    // special_left
    //   where a special row's name begins: as deep as the rows it is among.
    float special_left(const grid_row& _row) const
    {
        return name_x() + 6.0f +
               static_cast<float>(_row.depth) * ui_scale() * grid.indent;
    }

    ImU32 text_colour() const
    {
        return ImGui::GetColorU32(ImGuiCol_Text);
    }

    ImU32 muted_colour() const
    {
        return ImGui::GetColorU32(ImGuiCol_TextDisabled);
    }

    // how faded muted text is: the theme's, as its disabled text has it
    float muted_alpha() const
    {
        return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled).w;
    }

    // the grid's own ground: its panel, opaque
    ImU32 panel_colour() const
    {
        return (ImGui::GetColorU32(ImGuiCol_ChildBg) | IM_COL32_A_MASK);
    }

    void centred_text(
        ImVec2             _min,
        ImVec2             _max,
        const std::string& _text,
        ImU32              _colour
    )
    {
        const ImVec2 size = ImGui::CalcTextSize(_text.c_str());

        list->AddText(ImVec2(std::floor((_min.x + _max.x - size.x) * 0.5f),
                             std::floor((_min.y + _max.y - size.y) * 0.5f)),
                      _colour,
                      _text.c_str());

        return;
    }

    // where things are, and what they say
    spot        at(ImVec2 _point) const;
    float       name_left(const resolved_row& _row) const;
    float       child_left(const resolved_row& _holder) const;
    std::string shown_name(const resolved_row& _row) const;
    std::string value_text(const resolved_row& _row) const;
    int         names_by() const;
    bool        by_value(const resolved_row& _row) const;
    value_form  shown_form() const;
    std::string listed_value(const resolved_row& _row) const;
    std::string set_bits(const wide& _bits) const;
    std::string listed(const resolved_row& _row) const;
    float       label_width(const resolved_row& _row) const;
    std::string value_tip() const;
    std::string probe_text() const;
    float       fit_names() const;
    float       fit_values() const;
    int         cells_before(int _boundary) const;
    bool        renaming_here() const;
    bool        text_row(const grid_row& _row) const;
    ImU32       row_ground(const grid_row& _row) const;
    std::vector<const display_rule*> rules_for(const grid_row& _row) const;
    float       add_width() const;
    float       add_end() const;

    void rows_background();
    void row_cells(std::size_t _index, const grid_row& _row);
    void row_note(std::size_t _index, const grid_row& _row);
    void row_label(std::size_t _index, const grid_row& _row);
    void lines();
    void place_buttons();
    void name_buttons();
    void header();
    void interact();
    void respond();
    void open_menu(const spot& _under);
    bool field_place(const draft& _typed, ImVec2& _min, ImVec2& _max);
    void rename_field();
    void child_field();
    void row_drag();
    void drop_special(long _id, const spot& _under, bool _delivered);
    void tooltip(const grid_row& _row, const column& _column);
    void pinned_tooltip(const grid_row& _row, const spot& _under);
    void heading_tooltip(const spot& _under, ImVec2 _mouse);
    std::string definition(const resolved_row& _row) const;
    void remember() const;
};

/*
painter::at
  What lies under a point. The pinned columns' edges are found anywhere
down the grid, the nearer one when the two are close; a ruler boundary
only in the ruler, at a column's right edge with a cell left of it.
*/
spot
painter::at(
    ImVec2 _point
) const
{
    spot        out;
    const float top = view_min.y;

    out.header = (_point.y < top + l.header_h);
    out.band   = ( (out.header) &&
                   (_point.y < top + l.band_h) );
    out.ruler  = ( (out.header)                         &&
                   (l.ruler_h > 0.0f)                   &&
                   (_point.y >= top + l.band_h)         &&
                   (_point.y < top + l.band_h + l.ruler_h) );
    out.names  = (_point.x < cells_left);
    out.name = (l.name_w > 0.0f) &&
               (_point.x >= name_x()) &&
               (_point.x < name_x() + l.name_w);

    if (!out.header)
    {
        for (std::size_t index = 0; index < l.rows.size(); ++index)
        {
            if (_point.y >= row_y(index) &&
                _point.y < row_y(index) + l.row_h)
            {
                out.row = static_cast<int>(index);
                break;
            }
        }
    }

    if (!out.names)
    {
        out.column = column_at(l, _point.x - cells_x(0.0f));
    }

    // the pinned columns' edges
    const float reach      = EDGE_REACH * ui_scale();
    const float name_edge  = name_x() + l.name_w;
    const float value_edge = value_x() + l.value_w;
    const float to_name    = std::fabs(_point.x - name_edge);
    const float to_value   = std::fabs(_point.x - value_edge);

    if ( (l.value_w > 0.0f)    &&
         (to_value < reach)    &&
         (to_value <= to_name) )
    {
        out.edge = 2;
    }
    else if ((l.name_w > 0.0f) && (to_name < reach))
    {
        out.edge = 1;
    }

    // a boundary of the ruler's
    if ( (out.ruler) &&
         (out.edge == 0) )
    {
        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const float x = cells_x(l.columns[i].x + l.columns[i].w);

            if ( (x >= cells_left) &&
                 (std::fabs(_point.x - x) < reach) &&
                 (i >= first_plain) )
            {
                out.boundary = static_cast<int>(i);

                break;
            }
        }
    }

    // a button over the name column, unless an edge is as near
    if ( (out.header) &&
         (out.edge == 0) )
    {
        for (std::size_t i = 0; i < buttons.size(); ++i)
        {
            const name_button& each = buttons[i];

            if ( (_point.x >= each.min.x) &&
                 (_point.x <  each.max.x) &&
                 (_point.y >= each.min.y) &&
                 (_point.y <  each.max.y) )
            {
                out.by = static_cast<int>(i);

                break;
            }
        }
    }

    return out;
}

// painter::name_left
//   where a row's name begins: past its indent, a group's arrow, and its
// colour chip.
float
painter::name_left(
    const resolved_row& _row
) const
{
    const float line = ImGui::GetTextLineHeight();
    float       x    = name_x() + 6.0f +
                       static_cast<float>(_row.depth) * ui_scale() *
                           grid.indent;

    const row* source = find_row(doc, _row.id);
    if ((source != nullptr) && can_contain(*source))
    {
        x += line * 0.32f * 2.0f + 6.0f;
    }

    return x + line * 0.55f + 6.0f;
}

// painter::child_left
//   where the "Add child..." field under a group's or a range's rows
// begins: where a symbol's name one level deeper does.
float
painter::child_left(
    const resolved_row& _holder
) const
{
    const float line = ImGui::GetTextLineHeight();

    return name_x() + 6.0f +
           static_cast<float>(_holder.depth + 1) * ui_scale() * grid.indent +
           line * 0.55f + 6.0f;
}

// painter::shown_name
//   a row's name as the name column writes it, before it is fitted.
std::string
painter::shown_name(
    const resolved_row& _row
) const
{
    const std::string name =
        (_row.kind == row_kind::group)
            ? (_row.local.empty() ? std::string("group") : _row.local)
            : (grid.full_names ? _row.full : _row.local);

    return name.empty() ? std::string("(unnamed)") : name;
}

// painter::value_text
//   a row's value as the value column writes it, before it is fitted.
std::string
painter::value_text(
    const resolved_row& _row
) const
{
    if ( (_row.def == def_kind::field_value) &&
         (_row.kind == row_kind::symbol) )
    {
        return "= " + format_signed(_row.number);
    }

    if ( (_row.def == def_kind::literal) &&
         (_row.negative) &&
         (!is_wide(doc.width)) )
    {
        return format_signed(sign_extend(_row.value, doc.width));
    }

    // a wide sheet's macro is C's integer
    if ( (_row.def == def_kind::macro) &&
         (_row.kind == row_kind::symbol) &&
         (is_wide(doc.width)) )
    {
        return "0x" + group_digits(format_hex(_row.value, 0, true), 2);
    }

    return shown_wide(_row.bits, _row.fmt, doc.width);
}

// painter::names_by
//   what the name column lists each row by, as its buttons number it: 0
// its symbol name, 1 its own name, 2 its value.
int
painter::names_by() const
{
    if (grid.names_by_value)
    {
        return 2;
    }

    return grid.full_names ? 0 : 1;
}

// painter::by_value
//   whether the name column writes a row's value where its name would be:
// by value, a symbol's that has one. A group is no symbol, and keeps its
// name, as it does by symbol name.
bool
painter::by_value(
    const resolved_row& _row
) const
{
    return ( (grid.names_by_value)              &&
             (_row.kind != row_kind::group)     &&
             (_row.has_value) );
}

// painter::shown_form
//   the form the name column writes values in: the sheet's, or hex for a
// form a sheet as wide as this one does not write.
value_form
painter::shown_form() const
{
    return form_fits(grid.names_value_form, doc.width)
               ? grid.names_value_form
               : value_form::hex;
}

/*
painter::listed_value
  A row's value as the name column writes it, by value. As the header
writes it, it is the value column's own text; otherwise every row alike --
in hex, to the sheet's width; in decimal; in binary, from the nibble that
holds its highest set bit; as a run shifted into place, from its lowest
set bit -- or, for a value in a field, the number the field holds shifted
to the field's lowest bit -- the way the header writes masks; or as the
bits it sets.
*/
std::string
painter::listed_value(
    const resolved_row& _row
) const
{
    const wide& bits   = _row.bits;
    const bool  narrow = !is_wide(doc.width);

    switch (shown_form())
    {
        case value_form::written:
            break;

        case value_form::hex:
            return narrow ? shown_value(bits.low(), radix::hex, doc.width)
                          : "0x" + group_digits(format_hex(bits, 0, true), 2);

        case value_form::dec:
            return format_dec(bits.low());

        case value_form::bin:
        {
            const int highest = bits.highest();

            return (highest < 0)
                       ? std::string("0b0")
                       : "0b" + format_bin(bits.low(),
                                           (highest / 4 + 1) * 4,
                                           4,
                                           ' ');
        }

        case value_form::shift:
        {
            const bool field = ( (_row.def == def_kind::field_value) &&
                                 (_row.kind == row_kind::symbol) );
            const int  lsb   = field ? _row.span_lsb : bits.lowest();

            if (lsb < 0)
            {
                return "0";
            }

            const wide        run  = shift_right(bits, lsb);
            const std::string what =
                (run == wide(1u)) ? std::string("1")
              : run.fits(64)      ? "0x" + format_hex(run.low(), 1, true)
                                  : "0x" + format_hex(run, 0, true);

            return what + " << " + format_dec(static_cast<u64>(lsb));
        }

        case value_form::bits:
            return set_bits(bits);
    }

    return value_text(_row);
}

/*
painter::set_bits
  The bits a value sets, as runs, highest first, each named by the sheet's
numbering as the ruler names a run: "bit 4", "bits 5:4", "bits 31:28, 7";
past RUNS_MOST runs, how many more there are; "no bits" for none.
*/
std::string
painter::set_bits(
    const wide& _bits
) const
{
    // the runs, lowest first: each one's lowest and highest bit
    std::vector<std::pair<int, int>> runs;

    for (int low = _bits.next(0); low >= 0; )
    {
        int high = low;

        while ( (high + 1 < doc.width) &&
                (_bits.test(high + 1)) )
        {
            ++high;
        }

        runs.emplace_back(low, high);
        low = _bits.next(high + 1);
    }

    if (runs.empty())
    {
        return "no bits";
    }

    const bool  single = ( (runs.size() == 1) &&
                           (runs.front().first == runs.front().second) );
    std::string out    = single ? "bit " : "bits ";
    std::size_t named  = 0;

    for (auto at = runs.rbegin(); at != runs.rend(); ++at)
    {
        if (named == static_cast<std::size_t>(RUNS_MOST))
        {
            out += ", +" + format_dec(static_cast<u64>(runs.size() - named)) +
                   " more";

            break;
        }

        out += (named > 0) ? ", " : "";
        out += format_dec(static_cast<u64>(bit_label(doc, at->second)));

        if (at->second != at->first)
        {
            out += ":" + format_dec(static_cast<u64>(bit_label(doc,
                                                               at->first)));
        }

        ++named;
    }

    return out;
}

// painter::listed
//   what the name column writes for a row of the sheet's, before it is
// fitted: its value, by value, else its name.
std::string
painter::listed(
    const resolved_row& _row
) const
{
    return by_value(_row) ? listed_value(_row)
                          : shown_name(_row);
}

// painter::label_width
//   how wide the name column writes a row, past where its name begins: its
// value in the monospace font, by value, else its name.
float
painter::label_width(
    const resolved_row& _row
) const
{
    if (!by_value(_row))
    {
        return ImGui::CalcTextSize(shown_name(_row).c_str()).x;
    }

    ImGui::PushFont(mono_font(), 0.0f);

    const float width = ImGui::CalcTextSize(listed_value(_row).c_str()).x;

    ImGui::PopFont();

    return width;
}

// painter::value_tip
//   the value button's tooltip: before the column lists rows by value,
// what a press does, and that a press again writes the values another way;
// after, how they are written, and how the next press writes them.
std::string
painter::value_tip() const
{
    const value_form form = shown_form();
    const std::string way =
        FORM_WAYS[static_cast<std::size_t>(form)];

    if (names_by() != 2)
    {
        return "list each row by its value, " + way + "; a row without one "
               "keeps its name, faded, and a group its own\nonce it does, "
               "each press writes the values another way";
    }

    return "values " + way + "\na press writes them " +
           FORM_WAYS[static_cast<std::size_t>(next_form(form, doc.width))];
}

// painter::probe_text
//   the probe's value as the value column writes it.
std::string
painter::probe_text() const
{
    wide probe = doc.probe;

    probe.trim(doc.width);

    return shown_wide(probe,
                      (doc.gen.base == radix::inherit) ? radix::hex
                                                       : doc.gen.base,
                      doc.width);
}

/*
painter::fit_names
  The name column's width for its longest name, as the rows are now: indent,
arrow and chip included -- or, by value, for its longest value.
*/
float
painter::fit_names() const
{
    float widest = ImGui::CalcTextSize("symbol").x + 12.0f;

    for (const grid_row& each : l.rows)
    {
        float width = 0.0f;

        switch (each.kind)
        {
            case shown::probe:
            case shown::usage:
                width = special_left(each) - name_x() +
                        ImGui::CalcTextSize(special_label(each).c_str()).x;
                break;

            case shown::add:
                width = add_width();
                break;

            case shown::child:
                width = child_left(*each.resolved) - name_x() +
                        ImGui::CalcTextSize(ADD_CHILD_HINT).x +
                        ImGui::GetStyle().FramePadding.x * 2.0f;
                break;

            case shown::sheet:
                width = name_left(*each.resolved) - name_x() +
                        label_width(*each.resolved);
                break;
        }

        widest = std::max(widest, width + 12.0f);
    }

    return widest;
}

// painter::fit_values
//   the value column's width for its longest value.
float
painter::fit_values() const
{
    float widest = ImGui::CalcTextSize("value").x + 12.0f;

    ImGui::PushFont(mono_font(), 0.0f);

    for (const grid_row& each : l.rows)
    {
        std::string text;

        if (each.kind == shown::probe)
        {
            text = probe_text();
        }
        else if ( (each.kind == shown::sheet) &&
                  (each.resolved->has_value) )
        {
            text = value_text(*each.resolved);
        }

        if (!text.empty())
        {
            widest = std::max(widest,
                              ImGui::CalcTextSize(text.c_str()).x + 12.0f);
        }
    }

    ImGui::PopFont();

    // the usage row's summary, in the interface's font
    const std::string summary = format_dec(static_cast<u64>(doc.width)) +
                                " of " +
                                format_dec(static_cast<u64>(doc.width));

    return std::max(widest,
                    ImGui::CalcTextSize(summary.c_str()).x + 12.0f);
}

// painter::cells_before
//   how many cells -- whole bits, not runs -- lie left of a boundary.
int
painter::cells_before(
    int _boundary
) const
{
    int count = 0;

    for (int i = 0;
         ( (i <= _boundary) &&
           (i < static_cast<int>(l.columns.size())) );
         ++i)
    {
        const column& each = l.columns[static_cast<std::size_t>(i)];

        if ( (!each.collapsed) &&
             (!each.unused) )
        {
            ++count;
        }
    }

    return count;
}

// painter::renaming_here
//   whether a rename is being typed in the grid, rather than the tree.
bool
painter::renaming_here() const
{
    return ( (m.ui.rename_where == "grid") &&
             ( (m.ui.renaming != 0) ||
               (m.ui.renaming_segment >= 0) ) );
}

// painter::text_row
//   whether a row writes text across its cells instead of bits: a macro
// that takes arguments, one with no value, or a row with an error.
bool
painter::text_row(
    const grid_row& _row
) const
{
    if (_row.kind != shown::sheet)
    {
        return false;
    }

    const resolved_row& r = *_row.resolved;

    if ( (r.kind == row_kind::symbol) &&
         (r.def == def_kind::macro)   &&
         ( (function_like(*_row.source)) ||
           (!r.has_value) ) )
    {
        return true;
    }

    return !r.error.empty();
}

/*
painter::row_ground
  What a row's text and cells are drawn on before the cells' own fills: the
grid's panel, under the selection, the hover or a group's tint, as
rows_background lays them. Text over a row reads by it, so a selected row,
or a sheet's own selection colour, never hides its name.
*/
ImU32
painter::row_ground(
    const grid_row& _row
) const
{
    const ImU32 panel = panel_colour();

    // the probe, the usage and the add row lie on the panel itself
    if (_row.kind != shown::sheet)
    {
        return panel;
    }

    const resolved_row& r = *_row.resolved;

    if (is_selected(m, r.id))
    {
        return composite(to_u32(colours.selection), panel);
    }

    if (m.ui.hover_row == r.id)
    {
        return composite(to_u32(colours.hover), panel);
    }

    if (r.kind == row_kind::group)
    {
        return composite(to_u32(with_alpha(colours.hover,
                                           channel(colours.hover, 3) *
                                               0.6f)),
                         panel);
    }

    return panel;
}

// painter::add_width / add_end
//   how wide the add row's "+ Add new..." is, from the name column's left
// edge; and where it ends in the column, as cut by the column: as far as
// a press there adds.
float
painter::add_width() const
{
    return ( 6.0f + ImGui::CalcTextSize("+").x +
             ADD_NEW_GAP * ui_scale() +
             ImGui::CalcTextSize(ADD_NEW_LABEL).x + 4.0f );
}

float
painter::add_end() const
{
    return std::min(name_x() + l.name_w,
                    name_x() + add_width());
}

void
painter::rows_background()
{
    // the rows end where the columns do
    const float right = std::min(view_max.x,
                                 std::max(cells_left, cells_x(l.cells_w)));

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const grid_row& each = l.rows[i];
        const float     top  = row_y(i);

        // only the rows in view
        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        if ( (each.kind == shown::sheet) &&
             (is_selected(m, each.resolved->id)) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(colours.selection));
        }
        else if ( (each.kind == shown::sheet) &&
                  (m.ui.hover_row == each.resolved->id) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(colours.hover));
        }
        else if ( (each.kind == shown::sheet) &&
                  (each.resolved->kind == row_kind::group) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(with_alpha(colours.hover,
                                                  channel(colours.hover, 3) *
                                                      0.6f)));
        }
    }

    return;
}

/*
painter::rules_for
  The display rules over a row of the sheet's, in order -- those over its
groups too: what its cells are each asked of. None for any other row.
*/
std::vector<const display_rule*>
painter::rules_for(
    const grid_row& _row
) const
{
    if ( (_row.kind != shown::sheet) ||
         (grid.rules.empty()) )
    {
        return {};
    }

    std::vector<int>    lineage;
    const resolved_row* at = _row.resolved;

    while (at != nullptr)
    {
        lineage.push_back(at->id);
        at = m.res.find(at->parent);
    }

    return rules_over(grid, lineage);
}

/*
painter::row_cells
  A cell's fill says whether its bit is set; a range's or a value's field is
tinted underneath, so a value's zeros still show where it lives. A row with
no fill draws its set bits plain -- in the theme's text, faintly -- but for
a range, whose bits are all set and say nothing its name does not. Each
cell of a sheet's row shows its digit and its colour as the grid's display
rules over it say: a cell with its colour hidden is drawn as a row with no
fill is. A shared
bit in an exclusive group is outlined in the collision colour, and a
previewed operation's result is outlined over the present value. A row
with no bits has its cells drawn empty and writes no digits; row_note
writes what it is over them.
*/
void
painter::row_cells(
    std::size_t     _index,
    const grid_row& _row
)
{
    const float top    = row_y(_index);
    const float bottom = top + l.row_h;
    const float gap    = grid.show_cell_boxes ? (grid.cell_gap * 0.5f)
                                               : 0.0f;
    const float round  = grid.show_cell_boxes ? grid.rounding : 0.0f;
    const ImU32 empty  = to_u32(colours.empty_cell);

    wide        value;
    int         tint_low  = 0;   // the field a range or a value lives in
    int         tint_high = -1;
    const wide  none;
    const wide* overlap   = &none;
    rgba        colour    = colours.accent;
    bool        has       = false;
    bool        union_of  = false;
    wide        after;
    bool        ghost     = false;

    switch (_row.kind)
    {
        case shown::add:
        case shown::child:
            return;
        case shown::probe:
            value  = doc.probe;
            value.trim(doc.width);
            colour = colours.probe;
            has    = true;

            if ( (m.ui.op_preview)      &&
                 (m.preview_valid)      &&
                 (m.ui.preview_shown)   &&
                 (m.preview.probe) )
            {
                after = m.preview.probe_after;
                ghost = true;
            }

            break;

        case shown::usage:
            break;

        case shown::sheet:
        {
            const resolved_row& r = *_row.resolved;

            value     = r.bits;
            has       = r.has_value;
            colour    = set_colour(m, r);
            overlap   = &r.overlap;
            union_of  = (r.kind == row_kind::group);
            tint_low  = r.span_lsb;
            tint_high = (r.span_width > 0) ? (r.span_lsb + r.span_width - 1)
                                           : -1;
            ghost     = preview_after(m, r.id, after);

            // a row with no bits -- a macro that takes arguments, one with
            // no value, a row in error -- has its cells drawn empty, and
            // what it is written over them once the lines are drawn
            break;
        }
    }

    const bool  transparent = (colour == 0);

    // what its cells show: the grid's own settings -- a utility row keeps
    // its colour -- and, in a row of the sheet's, the rules over it
    const std::vector<const display_rule*> ruled = rules_for(_row);
    cell_shows                             plain;

    plain.digits = grid.show_digits;
    plain.names  = grid.show_labels;
    plain.colour = ( (_row.kind != shown::sheet) ||
                     (grid.show_colours) );

    const ImU32 fill   = to_u32(with_alpha(colour, union_of
                                                       ? colours.set_alpha *
                                                             0.45f
                                                       : colours.set_alpha));
    const ImU32 plain_set = faint(text_colour(),
                                  union_of ? PLAIN_SET * 0.6f : PLAIN_SET);
    const bool  a_range   = ( (_row.kind == shown::sheet) &&
                              (_row.resolved->kind == row_kind::symbol) &&
                              (_row.resolved->def == def_kind::range) );

    // a row with no bits writes no digits in its cells: it has no zeros
    const bool  blank  = text_row(_row);

    // whether a range's name, or a value's number, is written across its
    // span -- where the display rules show names over at least half of it
    // -- and on a chip of its colour, where they show that over at least
    // half; a range so named draws no plain fill, which would say nothing
    // more
    bool labelled   = false;
    bool plain_chip = transparent;

    if ( (!blank)                                    &&
         (_row.kind == shown::sheet)                 &&
         (_row.resolved->span_width > 1)             &&
         ( (_row.resolved->def == def_kind::range) ||
           (_row.resolved->def == def_kind::field_value) ) )
    {
        const resolved_row& r             = *_row.resolved;
        const int           high          = r.span_lsb + r.span_width - 1;
        int                 named_bits    = plain.names ? r.span_width : 0;
        int                 coloured_bits = plain.colour ? r.span_width : 0;

        if (!ruled.empty())
        {
            named_bits    = 0;
            coloured_bits = 0;

            for (int bit = r.span_lsb; bit <= high; ++bit)
            {
                const cell_shows here = shown_at(grid, ruled, bit, bit);

                named_bits    += here.names ? 1 : 0;
                coloured_bits += here.colour ? 1 : 0;
            }
        }

        labelled   = (named_bits * 2 >= r.span_width);
        plain_chip = ( (transparent) ||
                       (coloured_bits * 2 < r.span_width) );
    }
    const ImU32 under  = to_u32(with_alpha(colour, colours.range_alpha));
    const ImU32 picked = to_u32(with_alpha(colours.selection, 0.25f));
    const ImU32 text   = text_colour();
    const float faded  = muted_alpha();

    // what the cells lie on: the row's ground, and the empty cells' boxes
    const ImU32 ground = row_ground(_row);
    const ImU32 boxed  = grid.show_cell_boxes ? composite(empty, ground)
                                              : ground;

    // the ink last worked out, and the ground it was worked out for: a
    // row's cells lie on one or two grounds, and an ink is a few powers
    ImU32 inked_on  = 0;
    ImU32 inked     = text;
    bool  inked_any = false;

    for (std::size_t i = first_column; i < last_column; ++i)
    {
        const column& each = l.columns[i];
        const ImVec2  min(cells_x(each.x) + gap, top + gap);
        const ImVec2  max(cells_x(each.x + each.w) - gap, bottom - gap);

        if (_row.kind == shown::usage)
        {
            const std::vector<int>& counted = usage_of(_row);
            int                     most    = 0;

            for (int b = each.lsb; b <= each.msb; ++b)
            {
                most = std::max(most,
                                (static_cast<std::size_t>(b) < counted.size())
                                    ? counted[static_cast<std::size_t>(b)]
                                    : 0);
            }

            const ImU32 heat =
                (most == 0) ? empty
              : (most == 1) ? to_u32(with_alpha(colours.accent, 0.45f))
                            : to_u32(colours.collision);

            list->AddRectFilled(min, max, heat, round);

            if (most > 0)
            {
                centred_text(min, max, format_dec(static_cast<u64>(most)),
                             ink_on(composite(heat, ground), text));
            }

            continue;
        }

        // what the cell's text will lie on, built up as its fills are laid
        ImU32            beneath = boxed;
        const cell_shows here    = ruled.empty()
                                       ? plain
                                       : shown_at(grid, ruled, each.lsb,
                                                  each.msb);
        const bool       tinted  = ( (!transparent) && (here.colour) );

        if (grid.show_cell_boxes)
        {
            list->AddRectFilled(min, max, empty, round);
        }

        if ( (tinted) &&
             (each.lsb <= tint_high) &&
             (each.msb >= tint_low) )
        {
            list->AddRectFilled(min, max, under, round);
            beneath = composite(under, beneath);
        }

        const bool set = ( (has) &&
                           (any_in(value, each.lsb, each.msb)) );

        if ((set) && (tinted))
        {
            list->AddRectFilled(min, max, fill, round);
            beneath = composite(fill, beneath);
            ++painted[_index].coloured;
        }
        else if ( (set) &&
                  ( (!a_range) || (!labelled) ) )
        {
            list->AddRectFilled(min, max, plain_set, round);
            beneath = composite(plain_set, beneath);
            ++painted[_index].plain;
        }

        // An explicitly selected rectangular block is highlighted even
        // when its cells are empty or the row has no fill.
        if ((each.lsb <= m.ui.grid_rect_high) &&
            (each.msb >= m.ui.grid_rect_low) &&
            (_row.kind == shown::sheet) &&
            (std::find(m.ui.grid_rect_rows.begin(),
                       m.ui.grid_rect_rows.end(),
                       _row.resolved->id) != m.ui.grid_rect_rows.end()))
        {
            list->AddRectFilled(min, max, picked, round);
            list->AddRect(min, max, to_u32(colours.accent), round, 0, 1.2f);
            beneath = composite(picked, beneath);
        }

        // a collapsed or abbreviated column shows a digest of its bits, and
        // a cell its digit, in the ink that reads on the cell: a set bit's
        // full, a clear one's faded
        if ( (!blank) &&
             ( (each.collapsed) ||
               (each.unused)   ||
               (here.digits) ) )
        {
            if ( (!inked_any) ||
                 (inked_on != beneath) )
            {
                inked     = ink_on(beneath, text);
                inked_on  = beneath;
                inked_any = true;
            }

            const ImU32 ink = inked;

            centred_text(min,
                         max,
                         ( (each.collapsed) || (each.unused) )
                             ? cell_text(m, each, has ? value : none)
                             : std::string(set ? "1" : "0"),
                         set ? ink : faint(ink, faded));
            ++painted[_index].digits;
        }

        if (any_in(*overlap, each.lsb, each.msb))
        {
            list->AddRect(min, max, to_u32(colours.collision), round, 0,
                          2.0f);
        }

        if ( (ghost) &&
             (any_in(after, each.lsb, each.msb)) )
        {
            list->AddRect(ImVec2(min.x + 2.0f, min.y + 2.0f),
                          ImVec2(max.x - 2.0f, max.y - 2.0f),
                          to_u32(colours.ghost),
                          round,
                          0,
                          2.0f);
        }
    }

    // a range's name, or a value's number, across its span
    if (labelled)
    {
        const resolved_row& r     = *_row.resolved;
        float               left  = 1.0e9f;
        float               right = -1.0e9f;
        const int           high  = r.span_lsb + r.span_width - 1;

        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const column& each = l.columns[i];

            if ( (each.lsb <= high) &&
                 (each.msb >= r.span_lsb) )
            {
                left  = std::min(left, cells_x(each.x));
                right = std::max(right, cells_x(each.x + each.w));
            }
        }

        const std::string named =
            (r.def == def_kind::range)
                ? fit(grid.full_names ? r.full : r.local,
                      _row.source->short_name,
                      right - left - 8.0f)
                : fit(format_signed(r.number), "", right - left - 8.0f);

        if ( (!named.empty()) &&
             (right > left) )
        {
            const ImVec2 size = ImGui::CalcTextSize(named.c_str());
            const ImVec2 at(std::floor((left + right - size.x) * 0.5f),
                            std::floor(top + (l.row_h - size.y) * 0.5f));

            // the chip: the row's colour over its cells -- or, with no fill,
            // the row's own ground, which hides the cells' digits beneath the
            // name without giving the row a colour it does not have
            const ImU32 cells = composite(fill, composite(under, boxed));
            const ImU32 chip  = plain_chip
                                    ? ground
                                    : to_u32(with_alpha(colour, LABEL_CHIP));
            const ImU32 seen  = plain_chip ? ground
                                           : composite(chip, cells);

            list->AddRectFilled(ImVec2(at.x - 3.0f, at.y),
                                ImVec2(at.x + size.x + 3.0f, at.y + size.y),
                                chip,
                                3.0f);
            list->AddText(at, ink_on(seen, text), named.c_str());
        }
    }

    return;
}

/*
painter::row_note
  What a row with no bits writes over its empty cells: a macro, its
definition as the header writes it -- name, parameters and body, each
placeholder a macro name -- in the monospace font; a row in error, why.
It is centred in the cells in view, on a chip of the row's own ground that
covers the lines beneath it, in the ink that reads there.
*/
void
painter::row_note(
    std::size_t     _index,
    const grid_row& _row
)
{
    const resolved_row& r      = *_row.resolved;
    const bool          broken = !r.error.empty();
    std::string         note;

    if (broken)
    {
        note = "! " + r.error;
    }
    else
    {
        const bool        callable = function_like(*_row.source);
        const std::string body     = callable ? macro_body(m.res,
                                                           *_row.source,
                                                           nullptr)
                                   : r.expansion.empty()
                                       ? macro_body(m.res,
                                                    *_row.source,
                                                    nullptr)
                                       : r.expansion;
        const std::size_t first    = body.find('\n');

        note = r.full +
               (callable ? "(" + _row.source->params + ")" : std::string()) +
               " " + body.substr(0, first) +
               ((first != std::string::npos) ? " ..." : "");
    }

    noted[_index] = note;

    const float left  = std::max(cells_left, cells_x(0.0f));
    const float right = std::min(view_max.x, cells_x(l.cells_w));
    const float top   = row_y(_index);

    if (right - left < 24.0f)
    {
        return;
    }

    if (!broken)
    {
        ImGui::PushFont(mono_font(), 0.0f);
    }

    const std::string shown = fit(note, "", right - left - 16.0f);
    const ImVec2      size  = ImGui::CalcTextSize(shown.c_str());
    const ImVec2      at(std::floor((left + right - size.x) * 0.5f),
                         std::floor(top + (l.row_h - size.y) * 0.5f));
    const ImU32       chip  = row_ground(_row);

    list->AddRectFilled(ImVec2(at.x - 5.0f, at.y - 1.0f),
                        ImVec2(at.x + size.x + 5.0f, at.y + size.y + 1.0f),
                        chip,
                        3.0f);
    list->AddText(at,
                  broken ? legible(colours.collision, chip, text_colour())
                         : ink_on(chip, text_colour()),
                  shown.c_str());

    if (!broken)
    {
        ImGui::PopFont();
    }

    return;
}

void
painter::row_label(
    std::size_t     _index,
    const grid_row& _row
)
{
    const float top  = row_y(_index);
    const float text = top + (l.row_h - ImGui::GetTextLineHeight()) * 0.5f;
    const float left = name_x() + 6.0f;

    // the "Add child..." field is an item of its own, drawn over the grid
    if (_row.kind == shown::child)
    {
        return;
    }

    // the add row: its "+", and beside it, faded until the mouse is on it,
    // what a press there does
    if (_row.kind == shown::add)
    {
        if (l.name_w > 0.0f)
        {
            const float       after = left + ImGui::CalcTextSize("+").x +
                                      ADD_NEW_GAP * ui_scale();
            const std::string label = fit(ADD_NEW_LABEL,
                                          "",
                                          name_x() + l.name_w - after - 6.0f);

            list->AddText(ImVec2(left, text), text_colour(), "+");
            list->AddText(ImVec2(after, text),
                          add_hot ? text_colour()
                                  : faint(text_colour(), ADD_NEW_FADE),
                          label.c_str());
        }

        return;
    }

    if (_row.kind == shown::probe)
    {
        if (l.name_w > 0.0f)
        {
            const float at = special_left(_row);

            list->AddText(ImVec2(at, text),
                          legible(colours.probe, panel_colour(),
                                  text_colour()),
                          fit("probe", "",
                              name_x() + l.name_w - at - 6.0f).c_str());
        }

        if (l.value_w > 0.0f)
        {
            ImGui::PushFont(mono_font(), 0.0f);
            list->AddText(ImVec2(value_x() + 4.0f, text),
                          text_colour(),
                          fit(probe_text(), "", l.value_w - 8.0f).c_str());
            ImGui::PopFont();
        }

        return;
    }

    if (_row.kind == shown::usage)
    {
        if (l.name_w > 0.0f)
        {
            const float at = special_left(_row);

            list->AddText(ImVec2(at, text),
                          muted_colour(),
                          fit(special_label(_row), "",
                              name_x() + l.name_w - at - 6.0f).c_str());
        }

        if (l.value_w > 0.0f)
        {
            const std::string summary =
                format_dec(static_cast<u64>(allocated_of(_row))) +
                " of " + format_dec(static_cast<u64>(doc.width));

            list->AddText(ImVec2(value_x() + 4.0f, text),
                          muted_colour(),
                          summary.c_str());
        }

        return;
    }

    const resolved_row& r      = *_row.resolved;
    const float         indent = left + static_cast<float>(r.depth) *
                                            ui_scale() * grid.indent;
    const float         x      = name_left(r);

    // the ink that reads on the row: on a selected row, the sheet's own
    // selection colour may be dark in a light theme, or light in a dark one
    const ImU32         ink    = ink_on(row_ground(_row), text_colour());

    if (l.name_w > 0.0f)
    {
    // a group's arrow
    if (can_contain(*_row.source))
    {
        const float  size   = ImGui::GetTextLineHeight() * 0.32f;
        const ImVec2 centre(indent + size, top + l.row_h * 0.5f);
        const bool   open   = !_row.source->collapsed;

        const float lean = size * 0.6f;
        const float tip  = size * 0.8f;

        if (open)
        {
            list->AddTriangleFilled(ImVec2(centre.x - size, centre.y - lean),
                                    ImVec2(centre.x + size, centre.y - lean),
                                    ImVec2(centre.x, centre.y + tip),
                                    ink);
        }
        else
        {
            list->AddTriangleFilled(ImVec2(centre.x - lean, centre.y - size),
                                    ImVec2(centre.x - lean, centre.y + size),
                                    ImVec2(centre.x + tip, centre.y),
                                    ink);
        }
    }

    // the row's colour, as a chip before the name
    const float chip = ImGui::GetTextLineHeight() * 0.55f;
    const bool  chip_off = (_row.source != nullptr) && _row.source->transparent;

    if (!chip_off)
    {
        list->AddRectFilled(
            ImVec2(x - chip - 6.0f, top + (l.row_h - chip) * 0.5f),
            ImVec2(x - 6.0f, top + (l.row_h + chip) * 0.5f),
            to_u32(r.colour),
            2.0f);
    }

    // the name, unless a rename is being typed over it -- or, by value, the
    // row's value, in the monospace font and the ink the value column
    // writes it in: as the header writes it, where the name would be; every
    // row's alike, lined up at the column's right, its low digits kept. A
    // symbol without a value keeps its name, faded.
    if ( (m.ui.renaming != r.id) ||
         (!renaming_here()) )
    {
        const float room   = name_x() + l.name_w - x - 6.0f;
        const bool  valued = by_value(r);
        const bool  lined  = ( (valued) &&
                               (shown_form() != value_form::written) );
        const bool  faded  = ( (!valued)                     &&
                               (grid.names_by_value)         &&
                               (r.kind != row_kind::group) );

        written[_index] = listed(r);

        if (valued)
        {
            ImGui::PushFont(mono_font(), 0.0f);
        }

        const std::string shown =
            lined  ? fit_tail(written[_index], room)
          : valued ? fit(written[_index], "", room)
                   : fit(written[_index], _row.source->short_name, room);
        const float       wide_as = ImGui::CalcTextSize(shown.c_str()).x;
        const float       start   = lined ? (name_x() + l.name_w - 6.0f -
                                             wide_as)
                                          : x;

        list->AddText(ImVec2(start, text),
                      (!r.error.empty()) ? to_u32(colours.collision)
                      : faded            ? faint(ink, muted_alpha())
                                         : ink,
                      shown.c_str());
        written_end[_index] = start + wide_as;

        if (valued)
        {
            ImGui::PopFont();
        }
    }

    }

    if ( (l.value_w <= 0.0f) ||
         (!r.has_value) )
    {
        return;
    }

    ImGui::PushFont(mono_font(), 0.0f);

    const std::string fitted = fit(value_text(r), "", l.value_w - 8.0f);
    const ImVec2      size   = ImGui::CalcTextSize(fitted.c_str());

    list->AddText(ImVec2(value_x() + l.value_w - size.x - 6.0f,
                         text),
                  (r.kind == row_kind::group) ? faint(ink, muted_alpha())
                                              : ink,
                  fitted.c_str());
    ImGui::PopFont();

    return;
}

/*
painter::lines
  Vertical lines first, at each boundary between two columns: a segment
edge, else a major line, else a minor one -- one line per boundary, the most
important kind that is switched on. They run through every row: a row with
no bits has its cells like any other, and what it writes over them is
drawn after. They stop where the rows of the tree do, above the add row,
and start again at the first of the rows pinned at the bottom, wherever
those stand -- unless the sheet has its lines run down the room between,
the add row's and the room's below. Then the horizontal lines between
rows, heavier above groups, and the border last, around each run of rows.
*/
void
painter::lines()
{
    if (!grid.lines)
    {
        return;
    }

    const float top    = rows_top - l.ruler_h;
    const float bottom = row_y(l.rows.size());

    // the runs of rows the lines run down: every row, and the room between
    // them; or the rows of the tree, ending where the add row begins, and
    // the rows pinned at the bottom, wherever they stand
    std::vector<std::pair<float, float>> runs;

    if (grid.lines_past_rows)
    {
        runs.emplace_back(top, bottom);
    }
    else
    {
        runs.emplace_back(top, row_y(l.tree_end));

        if (l.pinned_bottom > 0)
        {
            runs.emplace_back(row_y(l.rows.size() - l.pinned_bottom),
                              bottom);
        }
    }

    // vertical
    //   one boundary's line, from its kind's top down through every row: a
    // row with no bits has its cells, and what it writes over them is drawn
    // after the lines, over them.
    auto vertical = [&](float            _x,
                        float            _from,
                        const line_spec& _spec)
    {
        for (std::size_t i = 0; i < runs.size(); ++i)
        {
            const float from = (i == 0) ? std::min(_from, runs[i].second)
                                        : runs[i].first;
            const float to   = runs[i].second;

            if (to > from)
            {
                draw_line(list, ImVec2(_x, from), ImVec2(_x, to), _spec);
            }
        }

        return;
    };

    for (std::size_t i = std::max<std::size_t>(1, first_column);
         i < last_column;
         ++i)
    {
        const column& left  = l.columns[i - 1];
        const column& right = l.columns[i];
        const float   x     = cells_x(right.x);

        if ( (x < cells_left) ||
             (x > view_max.x) )
        {
            continue;
        }

        // the bits above the boundary start at its higher side's lowest
        const int  edge    = std::max(left.lsb, right.lsb);
        const bool segment = (left.segment != right.segment);
        const bool major   = ( (grid.major_every > 0) &&
                               ((edge % grid.major_every) == 0) );

        if ( (segment) &&
             (grid.segments.show) )
        {
            vertical(x, top - l.band_h, grid.segments);
        }
        else if ( (major) &&
                  (grid.major.show) )
        {
            vertical(x, top, grid.major);
        }
        else if (grid.minor.show)
        {
            vertical(x, rows_top, grid.minor);
        }
    }

    const float right = std::max(cells_left, cells_x(l.cells_w));

    for (std::size_t i = 1; i < l.rows.size(); ++i)
    {
        const float y = row_y(i);

        if ( (y < rows_top) ||
             (y > view_max.y) )
        {
            continue;
        }

        const grid_row& each  = l.rows[i];
        const bool      group = ( (each.kind == shown::sheet) &&
                                  (each.resolved->kind == row_kind::group) );

        if ( (group) &&
             (grid.groups.show) )
        {
            draw_line(list, ImVec2(view_min.x, y), ImVec2(right, y),
                      grid.groups);
        }
        else if (grid.rows.show)
        {
            draw_line(list, ImVec2(view_min.x, y), ImVec2(right, y),
                      grid.rows);
        }
    }

    if (grid.border.show)
    {
        for (std::size_t i = 0; i < runs.size(); ++i)
        {
            const float  from = (i == 0) ? rows_top : runs[i].first;
            const ImVec2 min(cells_x(0.0f), from);
            const ImVec2 max(right, std::min(runs[i].second, view_max.y));

            if (max.y <= min.y)
            {
                continue;
            }

            draw_line(list, min, ImVec2(max.x, min.y), grid.border);
            draw_line(list, ImVec2(max.x, min.y), max, grid.border);
            draw_line(list, ImVec2(min.x, max.y), max, grid.border);
            draw_line(list, min, ImVec2(min.x, max.y), grid.border);
        }
    }

    return;
}

/*
painter::place_buttons
  The buttons over the name column, in the band strip above its label: side
by side from where the label begins, each as wide as its words, and
centred in the strip. A narrower column takes shorter words, and one too
narrow for the shortest has no buttons. They stop short of the column's
edge by more than its reach, so a drag of the edge is never a press on
one. In the two longer sets the value button names the form values are
written in while the column lists them, and is as wide as the widest
form's name, so it stays under the mouse from one press to the next.
*/
void
painter::place_buttons()
{
    buttons.clear();

    if ( (l.name_w <= 0.0f) ||
         (l.band_h <= 0.0f) )
    {
        return;
    }

    const float scale = ui_scale();
    const float pad   = BUTTON_PAD * scale;
    const float gap   = std::max(1.0f, std::floor(scale));
    const float left  = name_x() + 6.0f;
    const float right = name_x() + l.name_w -
                        std::max(6.0f, EDGE_REACH * scale + 2.0f);
    const char* form  = (names_by() == 2)
                            ? FORM_LABELS[static_cast<std::size_t>(
                                  shown_form())]
                            : NAMES_BY[0][2];

    ImGui::PushFont(nullptr, font_base() * BUTTON_TEXT);

    const float tall = std::floor(ImGui::GetTextLineHeight() +
                                  BUTTON_RISE * scale * 2.0f);
    const float top  = view_min.y + std::floor((l.band_h - tall) * 0.5f);

    // across
    //   how wide a button is for words.
    auto across = [pad](const char* _words)
    {
        return std::floor(ImGui::CalcTextSize(_words).x + pad * 2.0f);
    };

    for (std::size_t set = 0; set < 3; ++set)
    {
        const bool forms = (set < 2);
        float      wide_as[3];
        float      total = gap * 2.0f;

        for (std::size_t i = 0; i < 3; ++i)
        {
            wide_as[i] = across(NAMES_BY[set][i]);
        }

        for (const char* each : FORM_LABELS)
        {
            wide_as[2] = forms ? std::max(wide_as[2], across(each))
                               : wide_as[2];
        }

        total += wide_as[0] + wide_as[1] + wide_as[2];

        if (left + total > right)
        {
            continue;
        }

        float x = left;

        for (std::size_t i = 0; i < 3; ++i)
        {
            buttons.push_back(name_button{
                ImVec2(x, top),
                ImVec2(x + wide_as[i], top + tall),
                ( (i == 2) && (forms) ) ? form : NAMES_BY[set][i] });
            x += wide_as[i] + gap;
        }

        break;
    }

    ImGui::PopFont();

    return;
}

/*
painter::name_buttons
  The buttons over the name column: one bar of three, rounded at its ends.
The one chosen is filled with the accent and written in the ink that reads
on it; the others are fields, their words faded until the mouse is on one.
*/
void
painter::name_buttons()
{
    if (buttons.empty())
    {
        return;
    }

    const int   chosen = names_by();
    const float round  = std::min(ImGui::GetStyle().FrameRounding,
                                  (buttons.front().max.y -
                                   buttons.front().min.y) * 0.5f);

    ImGui::PushFont(nullptr, font_base() * BUTTON_TEXT);

    for (std::size_t i = 0; i < buttons.size(); ++i)
    {
        const name_button& each  = buttons[i];
        const bool         on    = (static_cast<int>(i) == chosen);
        const bool         hot   = (static_cast<int>(i) == button_hot);
        const ImU32        fill  = ImGui::GetColorU32(
                                       on  ? ImGuiCol_ButtonActive
                                     : hot ? ImGuiCol_FrameBgHovered
                                           : ImGuiCol_FrameBg);
        const ImU32        ink   = ink_on(composite(fill, panel_colour()),
                                          text_colour());
        const ImDrawFlags  round_at =
            (i == 0)                  ? ImDrawFlags_RoundCornersLeft
          : (i + 1 == buttons.size()) ? ImDrawFlags_RoundCornersRight
                                      : ImDrawFlags_RoundCornersNone;

        list->AddRectFilled(each.min, each.max, fill, round, round_at);
        centred_text(each.min,
                     each.max,
                     each.label,
                     (on || hot) ? ink : faint(ink, BUTTON_FADE));
    }

    ImGui::PopFont();

    return;
}

void
painter::header()
{
    const ImU32 background = ImGui::GetColorU32(ImGuiCol_ChildBg);
    const float top        = view_min.y;

    // the heading is pinned: it covers the rows scrolled beneath it
    list->AddRectFilled(ImVec2(view_min.x, top),
                        ImVec2(view_max.x, top + l.header_h),
                        background);

    // the bands: each run of columns with one segment, from the start of
    // the first one in view
    std::size_t start = first_column;

    while ( (start > 0)                                  &&
            (start < l.columns.size())                   &&
            (l.columns[start].segment >= 0)              &&
            (l.columns[start - 1].segment ==
             l.columns[start].segment) )
    {
        --start;
    }

    for (std::size_t i = start; i < last_column; )
    {
        const int   which = l.columns[i].segment;
        std::size_t end   = i;

        while ( (end < l.columns.size()) &&
                (l.columns[end].segment == which) )
        {
            ++end;
        }

        if (which >= 0)
        {
            const band&  each = l.bands[static_cast<std::size_t>(which)];
            const rgba   tone = (each.colour != 0) ? each.colour
                                                   : colours.accent;
            const ImVec2 min(cells_x(l.columns[i].x) + 1.0f, top + 2.0f);
            const ImVec2 max(cells_x(l.columns[end - 1].x +
                                     l.columns[end - 1].w) - 1.0f,
                             top + l.band_h - 2.0f);

            const ImU32 tint = to_u32(with_alpha(tone,
                                                 each.user >= 0 ? 0.40f
                                                                : 0.16f));

            list->AddRectFilled(min, max, tint, 4.0f);

            // its name, unless a rename is being typed over it, in the ink
            // that reads on the band: a segment's colour is the sheet's
            if ( (!renaming_here()) ||
                 (each.user < 0)    ||
                 (each.user != m.ui.renaming_segment) )
            {
                const std::string label =
                    (each.collapsed ? "+ " : "") +
                    fit(each.name, "", max.x - min.x - 16.0f);
                const ImU32       under = composite(tint, panel_colour());

                centred_text(min,
                             max,
                             label,
                             ink_on(under, text_colour()));
            }
        }

        i = end;
    }

    // the ruler: bit numbers, or a run's span
    if (grid.show_indices)
    {
        const float rtop = top + l.band_h;
        int         every = std::max(1, grid.index_every);

        // numbers wider than a cell -- a wide sheet's -- are thinned to
        // every second, fourth, ... bit, so they never run together
        ImGui::PushFont(nullptr, font_base() * 0.82f);

        const float widest =
            ImGui::CalcTextSize(format_dec(static_cast<u64>(
                                    std::max(bit_label(doc, 0),
                                             bit_label(doc,
                                                       doc.width - 1))))
                                    .c_str()).x + 6.0f;

        ImGui::PopFont();

        while ( (l.cell_w > 0.0f) &&
                (widest > l.cell_w * static_cast<float>(every)) &&
                (every < 4096) )
        {
            every *= 2;
        }

        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const column& each = l.columns[i];
            const ImVec2  min(cells_x(each.x), rtop);
            const ImVec2  max(cells_x(each.x + each.w), rtop + l.ruler_h);

            const bool hovered = ( (m.ui.hover_bit >= each.lsb) &&
                                   (m.ui.hover_bit <= each.msb) );

            if (hovered)
            {
                list->AddRectFilled(min, max, to_u32(colours.hover), 3.0f);
            }

            std::string label;

            if ( (each.collapsed) ||
                 (each.unused) )
            {
                label = format_dec(static_cast<u64>(
                            bit_label(doc, doc.msb_left ? each.msb
                                                        : each.lsb))) +
                        ":" +
                        format_dec(static_cast<u64>(
                            bit_label(doc, doc.msb_left ? each.lsb
                                                        : each.msb)));
                label = fit(label, "", each.w - 2.0f);
            }
            else if ((bit_label(doc, each.lsb) % every) == 0)
            {
                label = format_dec(static_cast<u64>(bit_label(doc,
                                                              each.lsb)));
            }

            ImGui::PushFont(nullptr, font_base() * 0.82f);
            centred_text(min, max, label,
                         hovered ? text_colour() : muted_colour());
            ImGui::PopFont();
        }

        // the boundary a drag sizes the cells by, lit
        const widening& widen = the_widening();
        const int       lit   = widen.active ? widen.boundary : boundary_hot;

        if ( (lit >= 0) &&
             (static_cast<std::size_t>(lit) < l.columns.size()) )
        {
            const column& each = l.columns[static_cast<std::size_t>(lit)];
            const float   x    = cells_x(each.x + each.w);

            list->AddLine(ImVec2(x, rtop),
                          ImVec2(x, rtop + l.ruler_h),
                          ImGui::GetColorU32(widen.active
                                                 ? ImGuiCol_SeparatorActive
                                                 : ImGuiCol_SeparatorHovered),
                          2.0f);
        }
    }

    // the corner, over both pinned parts
    list->AddRectFilled(view_min,
                        ImVec2(view_min.x + l.name_w + l.value_w,
                               top + l.header_h),
                        background);
    if (l.name_w > 0.0f)
    {
        const float line = ImGui::GetTextLineHeight();
        const float base = top + l.header_h - line - 3.0f;

        list->AddText(ImVec2(name_x() + 6.0f, base), muted_colour(),
                      "symbol");

        // sorted: an arrow at the column's right end, up for first to last
        // and down for the last first, and the key, where there is room
        if (grid.sort_by != sort_key::sheet)
        {
            const char* key   = SORT_WORDS[static_cast<std::size_t>(
                                    grid.sort_by)];
            const float word  = ImGui::CalcTextSize(key).x;
            const float arrow = line * 0.55f;
            const float right = name_x() + l.name_w - 8.0f;
            const float room  = l.name_w - ImGui::CalcTextSize("symbol").x -
                                arrow - 24.0f;
            const bool  wrote = (word <= room);
            const float x     = wrote ? (right - word - arrow - 4.0f)
                                      : (right - arrow);
            const float mid   = base + line * 0.5f;
            const ImU32 ink   = text_colour();

            if (grid.sort_descending)
            {
                list->AddTriangleFilled(ImVec2(x, mid - arrow * 0.4f),
                                        ImVec2(x + arrow, mid - arrow * 0.4f),
                                        ImVec2(x + arrow * 0.5f,
                                               mid + arrow * 0.45f),
                                        ink);
            }
            else
            {
                list->AddTriangleFilled(ImVec2(x, mid + arrow * 0.4f),
                                        ImVec2(x + arrow, mid + arrow * 0.4f),
                                        ImVec2(x + arrow * 0.5f,
                                               mid - arrow * 0.45f),
                                        ink);
            }

            if (wrote)
            {
                list->AddText(ImVec2(x + arrow + 4.0f, base), muted_colour(),
                              key);
            }
        }
    }

    // above it, the buttons that choose what the column lists rows by
    name_buttons();

    if (l.value_w > 0.0f)
    {
        list->AddText(ImVec2(value_x() + 4.0f,
                             top + l.header_h -
                                 ImGui::GetTextLineHeight() - 3.0f),
                      muted_colour(),
                      "value");
    }

    // the edges a drag resizes, lit while the mouse is on one or drags it
    const resize&   sizing = the_resize();
    const widening& widen  = the_widening();

    // edge
    //   one of the pinned columns' right edges: 1 the name's, 2 the value's.
    auto edge = [&](int   _which,
                    float _x)
    {
        const bool dragged = ( (sizing.active) &&
                               (sizing.column == ((_which == 2) ? "value"
                                                                : "name")) );
        const bool lit     = ( (dragged) ||
                               ( (edge_hot == _which) &&
                                 (!sizing.active)     &&
                                 (!widen.active) ) );

        list->AddLine(ImVec2(_x, view_min.y),
                      ImVec2(_x, view_max.y),
                      ImGui::GetColorU32(dragged ? ImGuiCol_SeparatorActive
                                       : lit     ? ImGuiCol_SeparatorHovered
                                                 : ImGuiCol_Separator),
                      lit ? 2.0f : 1.0f);

        return;
    };

    if (l.name_w > 0.0f) edge(1, name_x() + l.name_w);
    if (l.value_w > 0.0f) edge(2, value_x() + l.value_w);

    return;
}

void
painter::tooltip(
    const grid_row& _row,
    const column&   _column
)
{
    std::string text;

    if ( (_column.collapsed) ||
         (_column.unused) )
    {
        text = "bits " +
               format_dec(static_cast<u64>(bit_label(doc, _column.lsb))) +
               ".." +
               format_dec(static_cast<u64>(bit_label(doc, _column.msb))) +
               (_column.unused ? " -- unused, abbreviated"
                               : " -- a collapsed segment");
    }
    else
    {
        const int bit = _column.lsb;

        text = "bit " + format_dec(static_cast<u64>(bit_label(doc, bit))) +
               "  (byte " +
               format_dec(static_cast<u64>(byte_label(doc, bit))) + ")";

        // who claims this bit
        std::string claims;

        for (const resolved_row& each : m.res.rows)
        {
            if ( (each.kind == row_kind::symbol)          &&
                 (each.has_value)                         &&
                 ( (each.def == def_kind::bit) ||
                   (each.def == def_kind::range) )        &&
                 (each.bits.test(bit)) )
            {
                claims += (claims.empty() ? "" : ", ") + each.full;
            }
        }

        if (!claims.empty())
        {
            text += "\nallocated to " + claims;
        }
    }

    if ( (_row.kind == shown::sheet) &&
         (_row.resolved->has_value) )
    {
        const resolved_row& r = *_row.resolved;

        text += "\n" + r.full + " = " +
                (is_wide(doc.width)
                     ? runs_text(r.bits, 6)
                     : "0x" + group_digits(
                           format_hex(r.value, (doc.width + 3) / 4, true), 2));

        // a macro's value has a C type, and an expansion
        if (!r.ctype.empty())
        {
            text += "  (" + r.ctype + ")";
        }

        if (!r.expansion.empty())
        {
            text += "\nexpands to " +
                    ( (r.expansion.size() > 96)
                          ? r.expansion.substr(0, 94) + ".."
                          : r.expansion );
        }
    }

    ImGui::SetTooltip("%s", text.c_str());

    return;
}

/*
painter::definition
  What a row is, in a line: a bit's number, a range's bits, a value's number
and field, a literal, an expression or a macro.
*/
std::string
painter::definition(
    const resolved_row& _row
) const
{
    if (_row.kind == row_kind::group)
    {
        return "a group of " + format_dec(static_cast<u64>(_row.children)) +
               ((_row.children == 1) ? " row" : " rows");
    }

    const row* source = find_row(doc, _row.id);
    const int  low    = _row.span_lsb;
    const int  high   = _row.span_lsb + _row.span_width - 1;

    // bits
    //   a span by the sheet's numbering, high first.
    auto bits = [this, low, high]()
    {
        return (low == high)
                   ? "bit " + format_dec(static_cast<u64>(
                                  bit_label(doc, low)))
                   : "bits " + format_dec(static_cast<u64>(
                                   bit_label(doc, high))) + ".." +
                         format_dec(static_cast<u64>(bit_label(doc, low))) +
                         " (" + format_dec(static_cast<u64>(high - low + 1)) +
                         " bits)";
    };

    switch (_row.def)
    {
        case def_kind::bit:
            return (low >= 0) ? "a bit: " + bits() : "a bit";

        case def_kind::range:
            return (low >= 0) ? "a range: " + bits() : "a range";

        case def_kind::field_value:
        {
            const resolved_row* field = m.res.find(_row.field);

            return "the value " + format_signed(_row.number) + " in " +
                   ((field != nullptr) ? field->full : bits());
        }

        case def_kind::literal:
            return _row.negative ? "a literal, negative" : "a literal";

        case def_kind::expression:
            return "an expression: " +
                   ((source != nullptr) ? source->expr : std::string());

        case def_kind::macro:
            return "a macro" +
                   ( (source != nullptr) &&
                     ( (source->callable) ||
                       (!source->params.empty()) )
                         ? "(" + source->params + ")"
                         : std::string() ) +
                   ( _row.ctype.empty() ? std::string()
                                        : ", " + _row.ctype );
    }

    return std::string();
}

/*
painter::pinned_tooltip
  The name column's tooltip, once the mouse rests on a name: the macro name
in full, what the row is, and its brief; the value column's, the value in
every base, in the monospace font. The probe and the usage rows say what
they are.
*/
void
painter::pinned_tooltip(
    const grid_row& _row,
    const spot&     _under
)
{
    const int  index = _under.row;
    const long key   = 1000000L + static_cast<long>(index) * 2L +
                       (_under.name ? 0L : 1L);

    if (!rested(key))
    {
        return;
    }

    if (_row.kind == shown::probe)
    {
        ImGui::SetTooltip("the probe: a scratch value, %s\n"
                          "click its cells to flip bits; the operations "
                          "act on it when asked\n"
                          "drag it by its name among the rows; right-click "
                          "to pin it or add another",
                          probe_text().c_str());

        return;
    }

    if (_row.kind == shown::add)
    {
        ImGui::SetTooltip("Add a new symbol or show the utility rows");
        return;
    }

    if (_row.kind == shown::child)
    {
        ImGui::SetTooltip("type a name and press Enter: a bit of that name "
                          "in %s, at the next bit free there",
                          _row.resolved->full.c_str());

        return;
    }

    if (_row.kind == shown::usage)
    {
        const resolved_row* scope = (_row.scope != 0)
                                        ? m.res.find(_row.scope)
                                        : nullptr;

        ImGui::SetTooltip("usage: how many symbols %s allocate each bit -- "
                          "more than one in an exclusive group collides\n"
                          "drag it by its name among the rows -- in a group, "
                          "it counts the group's; right-click to pin it or "
                          "add another",
                          (scope != nullptr) ? ("in " + scope->full).c_str()
                                             : "in the sheet");

        return;
    }

    const resolved_row& r = *_row.resolved;

    if (_under.name)
    {
        std::string text = r.full.empty() ? shown_name(r) : r.full;

        text += "\n" + definition(r);

        if ( (_row.source != nullptr) &&
             (!_row.source->note.empty()) )
        {
            text += "\n\"" + _row.source->note + "\"";
        }

        if (!r.error.empty())
        {
            text += "\n" + r.error;
        }

        ImGui::SetTooltip("%s", text.c_str());

        return;
    }

    if (!r.has_value)
    {
        return;
    }

    ImGui::BeginTooltip();
    ImGui::PushFont(mono_font(), 0.0f);

    if (is_wide(doc.width))
    {
        // a set of bits: how many, where, and in hex, 128 bits a line
        const std::string hex = format_hex(r.bits, 0, true);
        const std::size_t line = 32;

        ImGui::Text("%d of %d bits set", r.bits.count(), doc.width);
        ImGui::TextUnformatted(runs_text(r.bits, 8).c_str());

        for (std::size_t at = 0; at < hex.size(); at += line)
        {
            ImGui::TextUnformatted(
                group_digits(hex.substr(at, line), 2).c_str());

            if (at >= line * 15)
            {
                ImGui::TextUnformatted("...");

                break;
            }
        }
    }
    else
    {
        // the value in every base
        const u64 value = r.value & width_mask(doc.width);

        ImGui::Text("hex      0x%s", group_digits(
            format_hex(value, (doc.width + 3) / 4, true), 2).c_str());
        ImGui::Text("unsigned %s", format_dec(value).c_str());

        if ((value >> (doc.width - 1)) & 1u)
        {
            ImGui::Text("signed   %s",
                        format_signed(sign_extend(value, doc.width)).c_str());
        }

        ImGui::Text("binary   %s",
                    format_bin(value, doc.width, 8, ' ').c_str());
    }

    ImGui::PopFont();

    if (!r.expansion.empty())
    {
        ImGui::TextUnformatted(("expands to " +
                                ( (r.expansion.size() > 96)
                                      ? r.expansion.substr(0, 94) + ".."
                                      : r.expansion )).c_str());
    }

    ImGui::EndTooltip();

    return;
}

/*
painter::heading_tooltip
  A band's tooltip says which bits it holds and what a click does to it; a
place in the ruler, the bit or the run of bits below it -- whose number
the ruler may have had no room to write -- and how the cells are sized; a
button over the name column, what it lists the rows by.
*/
void
painter::heading_tooltip(
    const spot& _under,
    ImVec2      _mouse
)
{
    const int at = (_under.names) ? -1
                                  : column_at(l, _mouse.x - cells_x(0.0f));

    // a button over the name column: what it lists the rows by -- and, the
    // value button, how it writes values
    if (_under.by >= 0)
    {
        if (rested(2000001L + static_cast<long>(_under.by)))
        {
            const std::string tip =
                (_under.by < 2) ? std::string(NAMES_BY_TIPS[
                                      static_cast<std::size_t>(_under.by)])
                                : value_tip();

            ImGui::SetTooltip("%s", tip.c_str());
        }

        return;
    }

    if ( (_under.names) &&
         (_under.ruler || _under.band || _under.header) )
    {
        if (rested(2000000L))
        {
            const std::string order =
                (grid.sort_by == sort_key::sheet)
                    ? std::string("the rows in the sheet's own order")
                    : std::string("each group's rows sorted by ") +
                          SORT_WORDS[static_cast<std::size_t>(grid.sort_by)] +
                          (grid.sort_descending ? ", the last first"
                                                : "");

            ImGui::SetTooltip("%s.\nright-click to sort them otherwise, "
                              "or to choose the columns; drag a column's "
                              "right edge to size it, and double-click the "
                              "edge to fit it to its text",
                              order.c_str());
        }

        return;
    }

    if (at < 0)
    {
        (void)rested(-1);

        return;
    }

    const column& each = l.columns[static_cast<std::size_t>(at)];
    const long    key  = 3000000L + static_cast<long>(at) * 2L +
                         (_under.band ? 0L : 1L);

    if (!rested(key))
    {
        return;
    }

    // span
    //   a run of bits by the sheet's numbering, high first.
    auto span = [this](int _low,
                       int _high)
    {
        return "bits " +
               format_dec(static_cast<u64>(bit_label(doc, _high))) + ".." +
               format_dec(static_cast<u64>(bit_label(doc, _low))) + " (" +
               format_dec(static_cast<u64>(_high - _low + 1)) + " bits)";
    };

    if ( (_under.band) &&
         (each.segment >= 0) )
    {
        const band& owner = l.bands[static_cast<std::size_t>(each.segment)];
        std::string text  = (owner.name.empty() ? std::string("a segment")
                                                : owner.name) +
                            ": " + span(owner.lsb, owner.msb);

        text += owner.collapsed
                    ? "\ncollapsed to one column: click to expand it"
                    : "\nclick to collapse it to one column";
        text += (owner.user >= 0)
                    ? "; double-click to rename it; right-click for more"
                    : "; double-click to name it as a segment of its own";

        ImGui::SetTooltip("%s", text.c_str());

        return;
    }

    std::string text;

    if ( (each.collapsed) ||
         (each.unused) )
    {
        text = span(each.lsb, each.msb) +
               (each.unused ? "\nunused, and abbreviated to one column"
                            : "\na collapsed segment");
    }
    else
    {
        text = "bit " + format_dec(static_cast<u64>(bit_label(doc,
                                                              each.lsb))) +
               ", in byte " +
               format_dec(static_cast<u64>(byte_label(doc, each.lsb)));
    }

    text += "\ndrag a boundary to size every cell; double-click one to "
            "put them back";

    ImGui::SetTooltip("%s", text.c_str());

    return;
}

/*
painter::interact
  One invisible button covers the content, and the view where the content
is smaller, so ImGui's own idea of hovered, pressed and dragged applies to
the grid as to any widget -- and a press beside the content, after a band
collapsed from under the mouse, still lands on the grid. What the button
reports is read once, before anything else is drawn. While a rename is
typed in the grid, the button lets the field drawn after it lie over it.
*/
void
painter::interact()
{
    ImGui::SetCursorScreenPos(origin);

    const ImVec2 room = ImGui::GetContentRegionAvail();

    // a field drawn after the button -- a rename's, or the "Add child..."
    // row's -- lies over it
    const bool child = std::any_of(l.rows.begin(),
                                   l.rows.end(),
                                   [](const grid_row& _row)
                                   {
                                       return (_row.kind == shown::child);
                                   });

    if ( (renaming_here()) ||
         (child) )
    {
        ImGui::SetNextItemAllowOverlap();
    }

    // as wide as the columns, and a margin when they overflow the view;
    // columns laid out to fill it fit it exactly, and scroll no way
    const float wide = l.name_w + l.value_w + l.cells_w;

    ImGui::InvisibleButton("##grid",
                           ImVec2(std::max({ 1.0f,
                                             room.x,
                                             (wide <= l.room + 0.5f)
                                                 ? wide
                                                 : wide + 8.0f }),
                                  std::max({ 1.0f,
                                             room.y,
                                             l.header_h +
                                                 static_cast<float>(
                                                     l.rows.size()) *
                                                     l.row_h + 8.0f })));

    over    = ImGui::IsItemHovered();
    pressed = ImGui::IsItemActivated();
    held    = ImGui::IsItemActive();
    twice   = ( (pressed) &&
                (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) );

    row_drag();
    respond();
    rename_field();
    child_field();

    return;
}

/*
painter::row_drag
  A row dragged by its name -- not by its arrow -- carries its id, as the
tree's rows do, so it drops here or in the tree. Dropped on a group's or a
range's name it goes into it, at the end; on a symbol's, just before it,
in its group: the row the drop would land on is lit as the mouse moves
over it. Only the sheet in front takes a drop: a row is no other sheet's.
The drag is the grid's button's, read straight after it; the drop, the
pinned columns'.
  A probe or usage row dragged by its name carries its id among the
special rows, and drops among the rows as a row does -- into a group or
range, or before a row; on another special row, just before it, placed as
it is; on the add row, after the sheet's rows; on the "Add child..." row,
at the end of its group's; and in the room below the rows, pinned at the
bottom.
*/
void
painter::row_drag()
{
    const ImVec2 mouse   = ImGui::GetMousePos();
    int&         named   = the_named();
    int&         carried = the_special();

    if (pressed)
    {
        const spot under = at(mouse);

        named   = 0;
        carried = 0;

        if ( (under.row >= 0)   &&
             (under.name)       &&
             (under.edge == 0)  &&
             (under.by < 0) )
        {
            const grid_row& each =
                l.rows[static_cast<std::size_t>(under.row)];

            // past the arrow, on the chip or the name
            if ( (each.kind == shown::sheet) &&
                 (mouse.x >= name_left(*each.resolved) -
                                 ImGui::GetTextLineHeight() * 0.55f - 6.0f) )
            {
                named = each.resolved->id;
            }
            else if ( ( (each.kind == shown::probe) ||
                        (each.kind == shown::usage) ) &&
                      (each.special != nullptr) )
            {
                carried = each.special->id;
            }
        }
    }

    if (!held)
    {
        named   = 0;
        carried = 0;
    }

    if ( (carried != 0) &&
         (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) )
    {
        const long         value = carried;
        const special_row* one   = find_special(grid, carried);

        ImGui::SetDragDropPayload(SPECIAL_DRAG, &value, sizeof(value));
        ImGui::TextUnformatted( ( (one != nullptr) &&
                                  (one->kind == special_kind::usage) )
                                    ? "allocated"
                                    : "probe");
        ImGui::EndDragDropSource();
    }

    if ( (named != 0) &&
         (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) )
    {
        const long          value = named;
        const resolved_row* shown = m.res.find(named);

        ImGui::SetDragDropPayload(ROW_DRAG, &value, sizeof(value));
        ImGui::TextUnformatted((shown != nullptr) ? shown->full.c_str()
                                                  : "?");
        ImGui::EndDragDropSource();
    }

    // the target is the pinned columns below the heading: an item of its
    // own, as a drag may not land on the item it began on
    const ImRect names(ImVec2(view_min.x, rows_top),
                       ImVec2(cells_left, view_max.y));

    if ( (!m.ui.in_front) ||
         (names.GetWidth() <= 0.0f) ||
         (!ImGui::BeginDragDropTargetCustom(names,
                                            ImGui::GetID("##rows.drop"))) )
    {
        return;
    }

    const ImGuiDragDropFlags accepting =
        ImGuiDragDropFlags_AcceptBeforeDelivery |
        ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ROW_DRAG,
                                                               accepting);
    const ImGuiPayload* moving  =
        (payload != nullptr) ? nullptr
                             : ImGui::AcceptDragDropPayload(SPECIAL_DRAG,
                                                            accepting);
    const spot          under   = at(mouse);

    if ( (moving != nullptr)                 &&
         (moving->DataSize == sizeof(long))  &&
         (under.names)                       &&
         (!under.header) )
    {
        drop_special(*static_cast<const long*>(moving->Data),
                     under,
                     moving->IsDelivery());
    }

    if ( (payload != nullptr)                     &&
         (payload->DataSize == sizeof(long))      &&
         (under.row >= 0)                         &&
         (under.names)                            &&
         (!under.header) )
    {
        const grid_row& each = l.rows[static_cast<std::size_t>(under.row)];

        if (each.kind == shown::sheet)
        {
            const long  dragged = *static_cast<const long*>(payload->Data);
            const float top     = row_y(static_cast<std::size_t>(under.row));
            const ImU32 lit     = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
            const bool  into    = ( (each.source != nullptr) &&
                                    (can_contain(*each.source)) );

            // into a group or range: its name lit; before a symbol: a line
            // above it, across the grid
            if (into)
            {
                list->AddRect(ImVec2(view_min.x + 1.0f, top + 1.0f),
                              ImVec2(cells_left - 1.0f, top + l.row_h - 1.0f),
                              lit,
                              3.0f,
                              ImDrawFlags_None,
                              2.0f);
            }
            else
            {
                list->AddLine(ImVec2(view_min.x, top),
                              ImVec2(view_max.x, top),
                              lit,
                              2.0f);
            }

            if ( (payload->IsDelivery()) &&
                 (dragged != each.resolved->id) )
            {
                option_set dropped;

                dropped.set("value", dragged);
                dropped.set("target", static_cast<long>(each.resolved->id));
                f.post("drop", std::move(dropped));
            }
        }
    }

    ImGui::EndDragDropTarget();

    return;
}

/*
painter::drop_special
  Where a special row dragged over the pinned columns would go, lit -- a
row's name outlined for into it, a line above a row for before it, a line
at the foot of the rows for at their end -- and, dropped, put there: one
"special.place", with where -- "top", "bottom" or "tree" -- the group it is
in and the row it comes before, and the special row it is put ahead of in
their order.
*/
void
painter::drop_special(
    long        _id,
    const spot& _under,
    bool        _delivered
)
{
    const ImU32   lit    = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
    special_place place  = special_place::bottom;
    int           parent = 0;
    int           before = 0;
    int           ahead  = 0;
    float         line   = -1.0f;   // the line's height, or a box's top
    bool          box    = false;

    if (_under.row < 0)
    {
        // the room below the rows: pinned at the bottom, above those there
        for (std::size_t i = l.rows.size() - l.pinned_bottom;
             i < l.rows.size();
             ++i)
        {
            if ( (l.rows[i].special != nullptr) &&
                 (l.rows[i].special->id != _id) )
            {
                ahead = l.rows[i].special->id;
                line  = row_y(i);

                break;
            }
        }

        if (line < 0.0f)
        {
            line = row_y(l.tree_end) + l.row_h;
        }
    }
    else
    {
        const std::size_t index = static_cast<std::size_t>(_under.row);
        const grid_row&   each  = l.rows[index];
        const float       top   = row_y(index);

        switch (each.kind)
        {
            case shown::sheet:
            {
                // into a group or range -- but its top quarter is before it
                const bool into = ( (each.source != nullptr)          &&
                                    (can_contain(*each.source))       &&
                                    (ImGui::GetMousePos().y >=
                                     top + l.row_h * 0.25f) );

                place  = special_place::tree;
                parent = into ? each.resolved->id : each.resolved->parent;
                before = into ? 0 : each.resolved->id;
                line   = top;
                box    = into;

                break;
            }

            case shown::probe:
            case shown::usage:
                if ( (each.special == nullptr) ||
                     (each.special->id == _id) )
                {
                    return;
                }

                place  = each.special->place;
                parent = each.special->parent;
                before = each.special->before;
                ahead  = each.special->id;
                line   = top;

                break;

            case shown::add:
                place = special_place::tree;
                line  = top;

                break;

            case shown::child:
                place  = special_place::tree;
                parent = each.resolved->id;
                line   = top + l.row_h;

                break;
        }
    }

    if (box)
    {
        list->AddRect(ImVec2(view_min.x + 1.0f, line + 1.0f),
                      ImVec2(cells_left - 1.0f, line + l.row_h - 1.0f),
                      lit,
                      3.0f,
                      ImDrawFlags_None,
                      2.0f);
    }
    else
    {
        list->AddLine(ImVec2(view_min.x, line),
                      ImVec2(view_max.x, line),
                      lit,
                      2.0f);
    }

    if (_delivered)
    {
        option_set placed;

        placed.set("value", _id);
        placed.set("place", std::string(place_name(place)));
        placed.set("parent", static_cast<long>(parent));
        placed.set("before", static_cast<long>(before));
        placed.set("ahead", static_cast<long>(ahead));
        f.post("special.place", std::move(placed));
    }

    return;
}

/*
painter::child_field
  The "Add child..." row's field, where the names of the rows one level
deeper begin, to the name column's edge -- drawn only while the whole row
is in view. Enter adds a bit of the name typed to the group or range; the
window gives the field the keyboard back, for the next.
*/
void
painter::child_field()
{
    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        if (l.rows[i].kind != shown::child)
        {
            continue;
        }

        const resolved_row& holder = *l.rows[i].resolved;
        const float         top    = row_y(i);
        const float         left   = child_left(holder) -
                                     ImGui::GetStyle().FramePadding.x;
        const float         right  = name_x() + l.name_w - 4.0f;
        const float         tall   = ImGui::GetTextLineHeight() +
                                     2.0f * std::max(0.0f, ui_scale());

        if ( (l.name_w <= 0.0f)            ||
             (top < rows_top)              ||
             (top + l.row_h > view_max.y)  ||
             (right - left < 24.0f) )
        {
            return;
        }

        option_set attrs;

        attrs.set("value", static_cast<long>(holder.id));
        attrs.set("indent", false);
        attrs.set("width", static_cast<double>(right - left));
        attrs.set("where", std::string("grid"));
        attrs.set("plus", m.ui.child_offer != 1);
        attrs.set("field", m.ui.child_offer != 2);
        attrs.set("fill", m.ui.child_fill);

        // the keyboard back, after a name added or refused here
        if ( (m.ui.adding_focus)              &&
             (m.ui.adding_to == holder.id)    &&
             (m.ui.adding_where == "grid") )
        {
            attrs.set("focus", true);
            attrs.set("text", m.ui.adding_text);
        }

        ImGui::SetCursorScreenPos(
            ImVec2(left, top + std::max(0.0f, (l.row_h - tall) * 0.5f)));
        ImGui::PushID("child");
        ImGui::PushID(holder.id);
        draw_add_child(attrs, f);
        ImGui::PopID();
        ImGui::PopID();

        return;
    }

    return;
}

/*
painter::respond
  Where a press lands decides what it does. A button over the name column
chooses what the column lists the rows by. An edge or a ruler boundary
starts a resize, and a double-click on one fits its column or puts the
cells back; a band collapses or expands, and a double-click renames it; a
name selects its row, an arrow opens or closes its group, and a
double-click on a name renames it; a cell starts a stroke, which a release
turns into one event. A right-click anywhere opens the menu for what it is
on.
*/
void
painter::respond()
{
    const ImVec2 mouse  = ImGui::GetMousePos();
    const spot   under  = at(mouse);
    stroke&      drag   = the_stroke();
    resize&      sizing = the_resize();
    widening&    widen  = the_widening();

    // a sheet just imported, or taken from the examples, has its name and
    // value columns fitted to its names and values
    if (m.ui.fit_columns)
    {
        option_set names;
        option_set values;

        names.set("target", std::string("name"));
        names.set("value", static_cast<double>(fit_names() / ui_scale()));
        names.set("arriving", true);
        f.post("grid.width", std::move(names));

        values.set("target", std::string("value"));
        values.set("value", static_cast<double>(fit_values() / ui_scale()));
        values.set("arriving", true);
        f.post("grid.width", std::move(values));
    }

    // a press on a button over the name column chooses what the column
    // lists the rows by, and starts no drag of the heading; presses one
    // after another are one step to undo. A press on the one chosen
    // changes nothing, but the value button's: listing rows by value, each
    // press writes them the next way
    if ( (pressed) &&
         (under.by >= 0) )
    {
        // the value button lists rows by value in the form they were last
        // written in, and, once it does, in the next
        const int   form   = static_cast<int>(
                                 (under.by == names_by())
                                     ? next_form(shown_form(), doc.width)
                                     : grid.names_value_form);
        const long  choice = (under.by == 2)
                                 ? static_cast<long>(NAMES_BY_VALUE + form)
                                 : static_cast<long>(under.by);
        option_set  payload;

        if ( (under.by != names_by()) ||
             (under.by == 2) )
        {
            payload.set("target", std::string("grid.names_by"));
            payload.set("value", choice);
            f.post("set", std::move(payload));
        }

        return;
    }

    // A heading drag swaps Symbols and Value. Edge drags retain their
    // existing resizing behavior. The bit visualizer stays to the right.
    header_drag& headings = the_header_drag();
    if (pressed && under.header && under.names &&
        under.edge == 0 && l.name_w > 0.0f && l.value_w > 0.0f)
    {
        headings.active = true;
        headings.names = under.name;
        headings.start_x = mouse.x;
    }
    if (headings.active && !held)
    {
        if (under.header && under.names && under.edge == 0 &&
            (headings.names != under.name) &&
            std::fabs(mouse.x - headings.start_x) > 5.0f)
            f.post("grid.swap_text", option_set{});
        headings.active = false;
        return;
    }
    if (headings.active)
        return;

    // a right-click opens the menu for what it is on
    if ( (over) &&
         (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) )
    {
        open_menu(under);

        return;
    }

    // the cursor says when a press would resize
    if ( (sizing.active) ||
         (widen.active)  ||
         ( (over) &&
           ( (under.edge != 0) || (under.boundary >= 0) ) ) )
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    // ... and when it would add, or choose what the name column lists
    else if ( (over) &&
              ( (add_hot) ||
                (button_hot >= 0) ) )
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // a press on an edge resizes its column; a double-click fits it
    if ( (pressed) &&
         (under.edge != 0) )
    {
        sizing.active = true;
        sizing.column = (under.edge == 2) ? "value" : "name";

        if (twice)
        {
            const float fitted = (under.edge == 2) ? fit_values()
                                                   : fit_names();
            option_set  payload;

            payload.set("target", sizing.column);
            payload.set("value", static_cast<double>(fitted / ui_scale()));
            f.post("grid.width", std::move(payload));
        }
    }
    // a press on a ruler boundary sizes the cells; a double-click resets
    else if ( (pressed) &&
              (under.boundary >= 0) )
    {
        widen.active   = true;
        widen.boundary = under.boundary;
        widen.width    = std::max(grid.cell_width, l.cell_w / ui_scale());
        widen.from     = mouse.x;
        widen.cells    = std::max(1, cells_before(under.boundary));
        widen.last     = widen.width;

        if (twice)
        {
            option_set payload;

            widen.width = grid_settings{}.cell_width;
            widen.last  = widen.width;
            payload.set("target", std::string("cells"));
            payload.set("value", static_cast<double>(widen.width));
            payload.set("reset", true);
            f.post("grid.width", std::move(payload));
        }
    }

    if (sizing.active)
    {
        const float delta = ImGui::GetIO().MouseDelta.x;

        // the sheet's widths are unscaled
        if (delta != 0.0f)
        {
            option_set payload;
            payload.set("value", static_cast<double>(delta / ui_scale()));
            payload.set("target", sizing.column);
            f.post("grid.resize", std::move(payload));
        }

        if (!held)
        {
            sizing.active = false;
        }

        return;
    }

    if (widen.active)
    {
        // the boundary follows the mouse: its movement, shared by the cells
        // left of it, and unscaled
        const float width = widen.width +
                            (mouse.x - widen.from) /
                                (static_cast<float>(widen.cells) *
                                 ui_scale());

        if (std::fabs(width - widen.last) >= 0.05f)
        {
            option_set payload;
            payload.set("target", std::string("cells"));
            payload.set("value", static_cast<double>(width));
            f.post("grid.width", std::move(payload));
            widen.last = width;
        }

        if (!held)
        {
            widen.active = false;
        }

        return;
    }

    // hovering: the row and the bit under the mouse
    if ( (over) &&
         (under.row >= 0) )
    {
        const grid_row& each = l.rows[static_cast<std::size_t>(under.row)];

        if (each.kind == shown::sheet)
        {
            m.ui.hover_row = each.resolved->id;
        }

        if ( (!under.names)              &&
             (under.column >= 0)         &&
             (under.edge == 0)           &&
             (each.kind != shown::add)   &&
             (each.kind != shown::child) )
        {
            const column& below = l.columns[static_cast<std::size_t>(
                under.column)];

            m.ui.hover_bit = below.lsb;
            tooltip(each, below);
        }
        else if ( (under.names) &&
                  (under.edge == 0) )
        {
            pinned_tooltip(each, under);
        }
    }
    else if ( (over)         &&
              (under.header) &&
              (under.edge == 0) )
    {
        heading_tooltip(under, mouse);
    }
    else
    {
        (void)rested(-1);
    }

    // the second press of a double-click on a band puts back what the
    // first did, and renames the band -- found by the first press, since
    // the band it collapsed or expanded has moved the columns under the
    // mouse since
    band_click& last = the_band_click();

    if ( (pressed)       &&
         (twice)         &&
         (last.valid)    &&
         (under.header)  &&
         (!under.names)  &&
         (ImGui::GetTime() - last.time <=
          static_cast<double>(ImGui::GetIO().MouseDoubleClickTime) + 0.05) )
    {
        option_set payload;

        payload.set("value", (last.user >= 0) ? last.user : last.created);
        payload.set("lsb", static_cast<long>(last.lsb));
        payload.set("msb", static_cast<long>(last.msb));
        f.post("grid.segment", payload);
        f.post("rename.segment", std::move(payload));
        last.valid = false;

        return;
    }

    // a press on a band collapses or expands it
    if ( (pressed)            &&
         (under.band)         &&
         (!under.names)       &&
         (under.column >= 0) )
    {
        const column& below = l.columns[static_cast<std::size_t>(
            under.column)];

        if (below.segment >= 0)
        {
            const band& each = l.bands[static_cast<std::size_t>(
                below.segment)];
            option_set  payload;

            payload.set("value", static_cast<long>(each.user));
            payload.set("lsb", static_cast<long>(each.lsb));
            payload.set("msb", static_cast<long>(each.msb));
            f.post("grid.segment", std::move(payload));

            last.valid   = true;
            last.time    = ImGui::GetTime();
            last.user    = each.user;
            last.created = (each.user < 0)
                               ? static_cast<long>(doc.segments.size())
                               : -1L;
            last.lsb     = each.lsb;
            last.msb     = each.msb;
        }

        return;
    }

    if ( (pressed) &&
         (under.row >= 0) )
    {
        const grid_row& each = l.rows[static_cast<std::size_t>(under.row)];

        // the add row's "+ Add new...", the plus or its label, opens the
        // menu of what to add
        if (each.kind == shown::add)
        {
            if ( (under.name) &&
                 (mouse.x < add_end()) )
            {
                f.post("context.add", option_set{});
            }

            return;
        }

        // the name column: an arrow opens or closes, a double-click on the
        // name renames, the rest selects
        if (under.names)
        {
            if (each.kind != shown::sheet)
            {
                return;
            }

            const resolved_row& r     = *each.resolved;
            const float         arrow = name_x() + 6.0f +
                                        static_cast<float>(r.depth) *
                                            ui_scale() * grid.indent +
                                        ImGui::GetTextLineHeight() * 0.8f;
            option_set          payload;

            payload.set("value", static_cast<long>(r.id));

            if ( (under.name) &&
                 (can_contain(*each.source)) &&
                 (mouse.x < arrow + 4.0f) )
            {
                payload.set("open", each.source->collapsed);
                f.post("toggle_open", std::move(payload));
            }
            else if ( (twice) &&
                      (under.name) )
            {
                f.post("rename.grid", std::move(payload));
            }
            else
            {
                f.post("select", std::move(payload));
            }

            return;
        }

        if (under.column < 0)
        {
            return;
        }

        const column& below = l.columns[static_cast<std::size_t>(
            under.column)];

        if ( (below.collapsed) ||
             (below.unused) )
        {
            return;
        }

        if (each.kind == shown::probe)
        {
            option_set payload;
            payload.set("value", static_cast<long>(below.lsb));
            f.post("grid.probe", std::move(payload));

            return;
        }

        if (each.kind != shown::sheet)
        {
            return;
        }

        const resolved_row& r = *each.resolved;

        drag.active = true;
        drag.row    = r.id;
        drag.row_to = r.id;
        drag.rectangle = ImGui::GetIO().KeyShift;
        drag.from   = below.lsb;
        drag.to     = below.lsb;
        drag.set    = !( (r.has_value) &&
                         (r.bits.test(below.lsb)) );

        if ((!drag.rectangle) && (!is_selected(m, r.id)))
        {
            option_set payload;
            payload.set("value", static_cast<long>(r.id));
            f.post("select", std::move(payload));
        }

        return;
    }

    if (!drag.active)
    {
        return;
    }

    // A rectangle follows the cursor's row as well as its column.
    if ((drag.rectangle) &&
        (under.row >= 0) &&
        (l.rows[static_cast<std::size_t>(under.row)].kind == shown::sheet))
    {
        drag.row_to = l.rows[static_cast<std::size_t>(under.row)].resolved->id;
    }

    // a stroke follows the mouse along its row, and ends on release
    if ( (under.column >= 0) &&
         (!l.columns[static_cast<std::size_t>(under.column)].collapsed) &&
         (!l.columns[static_cast<std::size_t>(under.column)].unused) )
    {
        drag.to = l.columns[static_cast<std::size_t>(under.column)].lsb;
    }

    // the stroke's extent, drawn while it lasts
    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        if (l.rows[i].kind != shown::sheet)
        {
            continue;
        }
        if (drag.rectangle)
        {
            int from_index = -1;
            int to_index   = -1;
            for (std::size_t k = 0; k < l.rows.size(); ++k)
            {
                if (l.rows[k].kind != shown::sheet) continue;
                if (l.rows[k].resolved->id == drag.row)
                    from_index = static_cast<int>(k);
                if (l.rows[k].resolved->id == drag.row_to)
                    to_index = static_cast<int>(k);
            }
            if ((from_index < 0) || (to_index < 0) ||
                (static_cast<int>(i) < std::min(from_index, to_index)) ||
                (static_cast<int>(i) > std::max(from_index, to_index)))
                continue;
        }
        else if (l.rows[i].resolved->id != drag.row)
        {
            continue;
        }

        const int low  = std::min(drag.from, drag.to);
        const int high = std::max(drag.from, drag.to);

        for (const column& each : l.columns)
        {
            if ( (each.lsb >= low) &&
                 (each.lsb <= high) &&
                 (!each.collapsed) &&
                 (!each.unused) )
            {
                list->AddRect(ImVec2(cells_x(each.x) + 1.0f, row_y(i) + 1.0f),
                              ImVec2(cells_x(each.x + each.w) - 1.0f,
                                     row_y(i) + l.row_h - 1.0f),
                              to_u32(colours.accent),
                              grid.rounding,
                              0,
                              2.0f);
            }
        }
    }

    if (!held)
    {
        option_set payload;
        payload.set("target", static_cast<long>(drag.row));

        if (drag.rectangle)
        {
            payload.set("last", static_cast<long>(drag.row_to));
            payload.set("low", static_cast<long>(std::min(drag.from,
                                                           drag.to)));
            payload.set("high", static_cast<long>(std::max(drag.from,
                                                            drag.to)));
            f.post("grid.rectangle", std::move(payload));
        }
        else if (drag.from == drag.to)
        {
            payload.set("value", static_cast<long>(drag.from));
            f.post("grid.toggle", std::move(payload));
        }
        else
        {
            payload.set("low", static_cast<long>(std::min(drag.from,
                                                          drag.to)));
            payload.set("high", static_cast<long>(std::max(drag.from,
                                                           drag.to)));
            payload.set("set", drag.set);
            f.post("grid.span", std::move(payload));
        }

        drag.active = false;
    }

    return;
}

/*
painter::open_menu
  A right-click on a button over the name column is about what the column
lists rows by; elsewhere on the pinned columns' heading, about the order
they show the rows in and what they show; on a band, about its segment; on
a probe or usage row, about where it is and how many there are; on a row
of the sheet's, about the row; anywhere else -- the add row, the empty
space below -- about the sheet.
*/
void
painter::open_menu(
    const spot& _under
)
{
    option_set payload;

    if (_under.by >= 0)
    {
        payload.set("value", static_cast<long>(_under.by));
        f.post("context.names_by", std::move(payload));

        return;
    }

    if ( (_under.header) &&
         (_under.names) )
    {
        payload.set("value", 0L);
        f.post("context.columns", std::move(payload));

        return;
    }

    if ( (_under.band)        &&
         (!_under.names)      &&
         (_under.column >= 0) &&
         (l.columns[static_cast<std::size_t>(_under.column)].segment >= 0) )
    {
        const band& each = l.bands[static_cast<std::size_t>(
            l.columns[static_cast<std::size_t>(_under.column)].segment)];

        payload.set("value", static_cast<long>(each.user));
        payload.set("lsb", static_cast<long>(each.lsb));
        payload.set("msb", static_cast<long>(each.msb));
        f.post("context.segment", std::move(payload));

        return;
    }

    if ( (_under.row >= 0) &&
         (l.rows[static_cast<std::size_t>(_under.row)].special != nullptr) )
    {
        payload.set("value",
                    static_cast<long>(
                        l.rows[static_cast<std::size_t>(_under.row)]
                            .special->id));
        f.post("context.special", std::move(payload));

        return;
    }

    if ( (_under.row >= 0) &&
         (l.rows[static_cast<std::size_t>(_under.row)].kind == shown::sheet) )
    {
        payload.set("value",
                    static_cast<long>(
                        l.rows[static_cast<std::size_t>(_under.row)]
                            .resolved->id));
        f.post("context.grid", std::move(payload));

        return;
    }

    payload.set("value", 0L);
    f.post("context.sheet", std::move(payload));

    return;
}

/*
painter::field_place
  Where a rename's field goes: over the row's name, or over the segment's
band. A row or a band out of view is scrolled to, and the field waits for
the frame it shows in; one that is no longer there gives the rename up.
*/
bool
painter::field_place(
    const draft& _typed,
    ImVec2&      _min,
    ImVec2&      _max
)
{
    const float scale = ui_scale();

    if (_typed.row != 0)
    {
        if (m.res.find(_typed.row) == nullptr)
        {
            f.post("rename.cancel", option_set{});

            return false;
        }

        for (std::size_t i = 0; i < l.rows.size(); ++i)
        {
            if ( (l.rows[i].kind != shown::sheet) ||
                 (l.rows[i].resolved->id != _typed.row) )
            {
                continue;
            }

            const float top    = row_y(i);
            const float bottom = top + l.row_h;

            // out of view: scrolled to, and shown the next frame
            if ( (top < rows_top) ||
                 (bottom > view_max.y) )
            {
                ImGui::SetScrollY(ImGui::GetScrollY() +
                                  ( (top < rows_top)
                                        ? (top - rows_top)
                                        : (bottom - view_max.y +
                                           ImGui::GetStyle().ScrollbarSize) ));

                return false;
            }

            _min = ImVec2(name_left(*l.rows[i].resolved) - 3.0f, top + 1.0f);
            _max = ImVec2(std::max(name_x() + l.name_w - 3.0f,
                                   _min.x + 80.0f * scale),
                          bottom - 1.0f);

            return true;
        }

        // hidden in a closed group a moment longer, while it opens
        return false;
    }

    if ( (_typed.segment < 0) ||
         (static_cast<std::size_t>(_typed.segment) >= doc.segments.size()) )
    {
        f.post("rename.cancel", option_set{});

        return false;
    }

    const float line = ImGui::GetTextLineHeight();
    float       left = 1.0e9f;
    float       right = -1.0e9f;

    for (const column& each : l.columns)
    {
        if ( (each.segment >= 0) &&
             (l.bands[static_cast<std::size_t>(each.segment)].user ==
              _typed.segment) )
        {
            left  = std::min(left, cells_x(each.x));
            right = std::max(right, cells_x(each.x + each.w));
        }
    }

    // a segment the grid does not show is named at the heading's start
    if (left > right)
    {
        left  = cells_left;
        right = cells_left + 160.0f * scale;
    }

    // a band out of view is scrolled to first
    if ( (right - left <= view_max.x - cells_left) &&
         ( (left < cells_left) || (right > view_max.x) ) )
    {
        ImGui::SetScrollX(ImGui::GetScrollX() +
                          ( (left < cells_left) ? (left - cells_left)
                                                : (right - view_max.x) ));

        return false;
    }

    // wide enough to type in, and inside the view
    const float wide = 140.0f * scale;

    if (right - left < wide)
    {
        const float middle = (left + right) * 0.5f;

        left  = middle - wide * 0.5f;
        right = middle + wide * 0.5f;
    }

    if (right > view_max.x)
    {
        left  -= right - view_max.x;
        right  = view_max.x;
    }

    left = std::max(left, cells_left);

    const float height = std::max(l.band_h, line + 4.0f);

    _min = ImVec2(left + 1.0f, view_min.y + 2.0f);
    _max = ImVec2(right - 1.0f, view_min.y + height - 2.0f);

    return true;
}

/*
painter::rename_field
  A rename asked for in the grid is typed where the name is shown. Enter,
or a click elsewhere after an edit, posts the new name; Escape, or a click
elsewhere with nothing typed, gives the rename up. The draft is kept here
between frames, and starts from the name each time a rename begins.
*/
void
painter::rename_field()
{
    draft& typed = the_draft();

    if (!renaming_here())
    {
        typed = draft{};

        return;
    }

    // a rename begun, or begun again, starts from the name
    if ( (m.ui.rename_focus)                   ||
         (typed.row != m.ui.renaming)          ||
         (typed.segment != m.ui.renaming_segment) )
    {
        typed         = draft{};
        typed.row     = m.ui.renaming;
        typed.segment = (m.ui.renaming != 0) ? -1 : m.ui.renaming_segment;
        typed.text    = m.ui.rename_text;
        typed.focus   = true;
    }

    ImVec2 min;
    ImVec2 max;

    if (!field_place(typed, min, max))
    {
        return;
    }

    const float line = ImGui::GetTextLineHeight();

    ImGui::SetCursorScreenPos(min);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(4.0f,
                               std::max(0.0f,
                                        (max.y - min.y - line) * 0.5f)));
    ImGui::PushID("rename");

    if (typed.focus)
    {
        ImGui::SetKeyboardFocusHere();
        typed.focus = false;
    }

    ImGui::SetNextItemWidth(max.x - min.x);

    const bool entered     = ImGui::InputText(
        "##name",
        &typed.text,
        ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll);
    const bool active      = ImGui::IsItemActive();
    const bool here        = ImGui::IsItemHovered();
    const bool deactivated = ImGui::IsItemDeactivated();
    const bool edited      = ImGui::IsItemDeactivatedAfterEdit();
    const bool escaped     = ImGui::IsKeyPressed(ImGuiKey_Escape, false);

    ImGui::PopID();
    ImGui::PopStyleVar();

    if (active)
    {
        typed.live = true;
    }

    const bool done    = ( (entered) ||
                           ( (deactivated) && (edited) && (!escaped) ) );
    const bool given_up =
        ( (!done) &&
          ( (deactivated) ||
            ( (escaped) && (!typed.live) ) ||
            ( (!typed.live) &&
              (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) &&
              (!here) ) ) );

    if (done)
    {
        option_set payload;

        payload.set("text", typed.text);

        if (typed.row != 0)
        {
            payload.set("value", static_cast<long>(typed.row));
            f.post("rename.commit", std::move(payload));
        }
        else
        {
            payload.set("value", static_cast<long>(typed.segment));
            f.post("rename.segment.commit", std::move(payload));
        }

        typed = draft{};
    }
    else if (given_up)
    {
        f.post("rename.cancel", option_set{});
        typed = draft{};
    }

    return;
}

// painter::remember
//   where this frame drew the grid's parts, for grid_spot.
void
painter::remember() const
{
    sighting& seen = the_sighting();

    seen.drawn    = true;
    seen.origin   = origin;
    seen.view_min = view_min;
    seen.name_w   = l.name_w;
    seen.value_w  = l.value_w;
    seen.values_first = l.values_first;
    seen.band_h   = l.band_h;
    seen.ruler_h  = l.ruler_h;
    seen.header_h = l.header_h;
    seen.row_h    = l.row_h;
    seen.end      = cells_x(l.cells_w);
    seen.room     = l.room;
    seen.columns  = l.columns;
    seen.ids.clear();
    seen.specials.clear();
    seen.tops.clear();
    seen.names.clear();
    seen.add      = -1;
    seen.child    = -1;
    seen.add_x    = name_x() + 6.0f + ImGui::CalcTextSize("+").x +
                    ADD_NEW_GAP * ui_scale() +
                    ImGui::CalcTextSize(ADD_NEW_LABEL).x * 0.5f;
    seen.buttons.clear();
    seen.labels   = written;
    seen.ends     = written_end;
    seen.notes    = noted;
    seen.painted  = painted;

    for (const name_button& each : buttons)
    {
        seen.buttons.push_back(ImVec2((each.min.x + each.max.x) * 0.5f,
                                      (each.min.y + each.max.y) * 0.5f));
    }

    for (const grid_row& each : l.rows)
    {
        const bool sheet_row = (each.kind == shown::sheet);

        if (each.kind == shown::add)
        {
            seen.add = static_cast<int>(seen.ids.size());
        }

        if (each.kind == shown::child)
        {
            seen.child = static_cast<int>(seen.ids.size());
        }

        seen.tops.push_back(row_y(seen.ids.size()));
        seen.ids.push_back(sheet_row ? each.resolved->id : 0);
        seen.specials.push_back((each.special != nullptr) ? each.special->id
                                                          : 0);

        // a special row: what it says, in view or not, for grid_special
        if ( (each.special != nullptr) &&
             (seen.ids.size() <= seen.labels.size()) )
        {
            const std::size_t at = seen.ids.size() - 1;

            seen.labels[at] = special_label(each);
            seen.notes[at]  = (each.kind == shown::probe)
                                  ? probe_text()
                                  : format_dec(static_cast<u64>(
                                        allocated_of(each))) +
                                        " of " +
                                        format_dec(static_cast<u64>(
                                            doc.width));
        }
        seen.names.push_back(sheet_row ? name_left(*each.resolved)
                           : (each.special != nullptr) ? special_left(each)
                                                       : name_x() + 6.0f);
    }

    return;
}

void
painter::run()
{
    origin   = ImGui::GetCursorScreenPos();
    view_min = ImGui::GetWindowPos();
    view_max = ImVec2(view_min.x + ImGui::GetWindowSize().x,
                      view_min.y + ImGui::GetWindowSize().y);

    cells_left = view_min.x + l.name_w + l.value_w;
    rows_top   = view_min.y + l.header_h;

    // the columns in view: from the first whose right edge is past the
    // pinned columns, to the first whose left edge is past the view
    const float shown_from = cells_left - cells_x(0.0f);
    const float shown_to   = view_max.x - cells_x(0.0f);

    first_column = static_cast<std::size_t>(
        std::upper_bound(l.columns.begin(), l.columns.end(), shown_from,
                         [](float _at, const column& _each)
                         {
                             return (_at < _each.x + _each.w);
                         }) -
        l.columns.begin());
    last_column  = static_cast<std::size_t>(
        std::upper_bound(l.columns.begin(), l.columns.end(), shown_to,
                         [](float _at, const column& _each)
                         {
                             return (_at < _each.x);
                         }) -
        l.columns.begin());
    first_plain  = l.columns.size();

    for (std::size_t i = 0; i < l.columns.size(); ++i)
    {
        if ( (!l.columns[i].collapsed) &&
             (!l.columns[i].unused) )
        {
            first_plain = i;

            break;
        }
    }

    // the buttons over the name column, which what follows finds the mouse
    // on; and room for what the rows write
    place_buttons();
    written.assign(l.rows.size(), std::string());
    written_end.assign(l.rows.size(), 0.0f);
    noted.assign(l.rows.size(), std::string());
    painted.assign(l.rows.size(), cells_drawn{});

    // what the mouse is over, so the edges it would drag can light up, the
    // add row's label show that a press there adds, and a button light
    if (ImGui::IsWindowHovered())
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        const spot   under = at(mouse);

        edge_hot     = under.edge;
        boundary_hot = under.boundary;
        add_hot      = ( (under.row >= 0)                                 &&
                         (l.rows[static_cast<std::size_t>(under.row)].kind ==
                          shown::add)                                     &&
                         (under.name)                                     &&
                         (under.edge == 0)                                &&
                         (mouse.x < add_end()) );
        button_hot   = under.by;
    }

    // the rows region: below the heading
    list->PushClipRect(ImVec2(view_min.x, rows_top), view_max, true);
    rows_background();

    // the cells: right of the pinned columns
    list->PushClipRect(ImVec2(cells_left, rows_top), view_max, true);

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const float top = row_y(i);

        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        row_cells(i, l.rows[i]);
    }

    // the hovered column, across every row
    if (m.ui.hover_bit >= 0)
    {
        for (std::size_t c = first_column; c < last_column; ++c)
        {
            const column& each = l.columns[c];

            if ( (m.ui.hover_bit >= each.lsb) &&
                 (m.ui.hover_bit <= each.msb) )
            {
                list->AddRectFilled(ImVec2(cells_x(each.x), rows_top),
                                    ImVec2(cells_x(each.x + each.w),
                                           row_y(l.rows.size())),
                                    to_u32(colours.hover));
            }
        }
    }

    list->PopClipRect();

    // the pinned name and value columns, over cells scrolled beneath them
    list->PushClipRect(ImVec2(view_min.x, rows_top),
                       ImVec2(cells_left, view_max.y),
                       true);
    list->AddRectFilled(ImVec2(view_min.x, rows_top),
                        ImVec2(cells_left, view_max.y),
                        ImGui::GetColorU32(ImGuiCol_ChildBg));
    rows_background();

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const float top = row_y(i);

        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        row_label(i, l.rows[i]);
    }

    list->PopClipRect();

    // the lines, over the cells and through the pinned columns
    list->PushClipRect(ImVec2(view_min.x, view_min.y), view_max, true);
    lines();
    list->PopClipRect();

    // what a row with no bits writes over its cells, over the lines
    list->PushClipRect(ImVec2(cells_left, rows_top), view_max, true);

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const float top = row_y(i);

        if ( (text_row(l.rows[i]))      &&
             (top + l.row_h >= rows_top) &&
             (top <= view_max.y) )
        {
            row_note(i, l.rows[i]);
        }
    }

    list->PopClipRect();
    list->PopClipRect();

    // the heading, pinned over all of it
    list->PushClipRect(view_min, view_max, true);
    header();

    if ( (grid.lines) &&
         (grid.segments.show) )
    {
        // the band edges again, above the heading's fill
        for (std::size_t i = std::max<std::size_t>(1, first_column);
             i < last_column;
             ++i)
        {
            if (l.columns[i - 1].segment != l.columns[i].segment)
            {
                const float x = cells_x(l.columns[i].x);

                if (x >= cells_left)
                {
                    draw_line(list,
                              ImVec2(x, view_min.y),
                              ImVec2(x, rows_top),
                              grid.segments);
                }
            }
        }
    }

    list->PopClipRect();

    interact();
    remember();

    return;
}

}  // anonymous namespace


/*
draw_grid
  The grid fills what is left of its panel, in a child window of its own so
it scrolls by itself.
*/
void
draw_grid(
    model&       _model,
    imgui_frame& _frame
)
{
    // this document's state, while its grid is drawn
    drawing()    = _model.id;
    drawn_last() = _model.id;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));

    if (ImGui::BeginChild("##bitgrid",
                          ImVec2(0.0f, 0.0f),
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        // the width to fill: the view's, less a vertical scroll bar
        const float  room  = ImGui::GetCurrentWindow()->InnerRect.GetWidth();
        const layout shape = lay_out(_model, room);
        painter      paint(_model, _frame, shape);

        paint.run();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    return;
}

/*
grid_spot
  The last frame's geometry, asked about by part: a row's name or a bit's
cell by the row's id -- a special row's, "special" and "special.cell", by
its id among the special rows -- a band or a ruler boundary by a bit it
holds, the pinned columns' edges and the buttons over the name column by
name; and "below", the room below the rows of the tree, where there is
any.
*/
grid_point
grid_spot(
    const std::string& _part,
    int                _row,
    int                _bit
)
{
    return grid_spot(drawn_last(), _part, _row, _bit);
}

grid_point
grid_spot(
    int                _document,
    const std::string& _part,
    int                _row,
    int                _bit
)
{
    const sighting& seen = states()[_document].seen;
    grid_point      out;

    if (!seen.drawn)
    {
        return out;
    }

    // the row's index, and the column a bit is shown in
    int           index   = -1;
    const column* holding = nullptr;

    const bool special = (_part.compare(0, 7, "special") == 0);

    for (std::size_t i = 0; i < seen.ids.size(); ++i)
    {
        const int id = special ? seen.specials[i] : seen.ids[i];

        if ( (_row != 0) &&
             (id == _row) )
        {
            index = static_cast<int>(i);

            break;
        }
    }

    for (const column& each : seen.columns)
    {
        if ( (_bit >= each.lsb) &&
             (_bit <= each.msb) )
        {
            holding = &each;

            break;
        }
    }

    const float cells  = seen.origin.x + seen.name_w + seen.value_w;
    const float middle = (index >= 0)
                             ? seen.tops[static_cast<std::size_t>(index)] +
                                   seen.row_h * 0.5f
                             : 0.0f;
    const float first  = seen.view_min.y + seen.header_h + seen.row_h * 0.5f;

    if ( ( (_part == "name") ||
           (_part == "special") ) &&
         (index >= 0) )
    {
        out = grid_point{ true,
                          seen.names[static_cast<std::size_t>(index)] + 4.0f,
                          middle };
    }
    else if ( (_part == "below") &&
              (seen.add >= 0) )
    {
        // between the add row and the first row after it
        const float from = seen.tops[static_cast<std::size_t>(seen.add)] +
                           seen.row_h;
        const float to   = (static_cast<std::size_t>(seen.add) + 1 <
                            seen.tops.size())
                               ? seen.tops[static_cast<std::size_t>(seen.add) +
                                           1]
                               : from;

        out = grid_point{ to - from >= seen.row_h,
                          seen.view_min.x + 12.0f,
                          (from + to) * 0.5f };
    }
    else if ( ( (_part == "cell") ||
                (_part == "special.cell") ) &&
              (index >= 0)      &&
              (holding != nullptr) )
    {
        out = grid_point{ true, cells + holding->x + holding->w * 0.5f,
                          middle };
    }
    else if ( (_part == "band")       &&
              (holding != nullptr)    &&
              (seen.band_h > 0.0f) )
    {
        out = grid_point{ true, cells + holding->x + holding->w * 0.5f,
                          seen.view_min.y + seen.band_h * 0.5f };
    }
    else if ( (_part == "boundary")   &&
              (holding != nullptr)    &&
              (seen.ruler_h > 0.0f) )
    {
        out = grid_point{ true, cells + holding->x + holding->w,
                          seen.view_min.y + seen.band_h +
                              seen.ruler_h * 0.5f };
    }
    else if (_part == "edge.name")
    {
        out = grid_point{ seen.name_w > 0.0f,
                          seen.view_min.x + seen.name_w +
                          (seen.values_first ? seen.value_w : 0.0f),
                          first };
    }
    else if (_part == "edge.value")
    {
        out = grid_point{ seen.value_w > 0.0f,
                          seen.view_min.x + seen.value_w +
                          (seen.values_first ? 0.0f : seen.name_w),
                          first };
    }
    else if ( (_part == "heading") &&
              (seen.name_w > 0.0f) )
    {
        // where "symbol" is written, under the buttons over the column
        out = grid_point{ true,
                          seen.view_min.x + 12.0f +
                              (seen.values_first ? seen.value_w : 0.0f),
                          seen.view_min.y + seen.header_h - 6.0f };
    }
    else if (_part == "end")
    {
        out = grid_point{ true, seen.end, first };
    }
    else if (_part == "room")
    {
        out = grid_point{ true, seen.view_min.x + seen.room, first };
    }
    else if ( (_part == "child") &&
              (seen.child >= 0) &&
              (seen.name_w > 0.0f) )
    {
        out = grid_point{ true,
                          seen.view_min.x + seen.name_w * 0.5f,
                          seen.origin.y + seen.header_h +
                              (static_cast<float>(seen.child) + 0.5f) *
                                  seen.row_h };
    }
    else if ( (_part == "add") &&
              (seen.add >= 0) &&
              (seen.name_w > 0.0f) )
    {
        out = grid_point{ true,
                          seen.add_x,
                          seen.origin.y + seen.header_h +
                              (static_cast<float>(seen.add) + 0.5f) *
                                  seen.row_h };
    }
    else if (_part.compare(0, 9, "names_by.") == 0)
    {
        const std::string which  = _part.substr(9);
        const std::size_t button = (which == "symbol") ? 0u
                                 : (which == "name")   ? 1u
                                 : (which == "value")  ? 2u
                                                       : 3u;

        if (button < seen.buttons.size())
        {
            out = grid_point{ true,
                              seen.buttons[button].x,
                              seen.buttons[button].y };
        }
    }

    return out;
}

/*
grid_written
  What the last frame wrote for a row of the sheet's, found by its id among
the rows it drew: the name column's text before it was fitted, and where
the fitted text ended; and what a row with no bits wrote over its cells.
*/
grid_text
grid_written(
    int _row
)
{
    return grid_written(drawn_last(), _row);
}

bool
grid_pinned_edge(
    int    _document,
    float& _x
)
{
    const auto found = states().find(_document);

    if ( (found == states().end())                          ||
         (!found->second.seen.drawn)                        ||
         (found->second.seen.name_w + found->second.seen.value_w <= 0.0f) )
    {
        return false;
    }

    const sighting& seen = found->second.seen;

    _x = seen.view_min.x + seen.name_w + seen.value_w;

    return true;
}

/*
grid_special
  What the last frame wrote for a special row, found by its id among the
special rows: its name -- "probe", "allocated", "allocated in" a group --
and its value: the probe's, or how many bits it counts allocated, "N of
W".
*/
grid_text
grid_special(
    int _special
)
{
    const sighting& seen = states()[drawn_last()].seen;
    grid_text       out;

    for (std::size_t i = 0; i < seen.specials.size(); ++i)
    {
        if ( (_special != 0)              &&
             (seen.specials[i] == _special) &&
             (i < seen.labels.size()) )
        {
            out.found = true;
            out.label = seen.labels[i];
            out.note  = (i < seen.notes.size()) ? seen.notes[i]
                                                : std::string();

            break;
        }
    }

    return out;
}

grid_text
grid_written(
    int _document,
    int _row
)
{
    const sighting& seen = states()[_document].seen;
    grid_text       out;

    for (std::size_t i = 0; i < seen.ids.size(); ++i)
    {
        if ( (_row != 0)                 &&
             (seen.ids[i] == _row)       &&
             (i < seen.labels.size())    &&
             (!seen.labels[i].empty()) )
        {
            out.found = true;
            out.label = seen.labels[i];
            out.end   = (i < seen.ends.size()) ? seen.ends[i] : 0.0f;
            out.note  = (i < seen.notes.size()) ? seen.notes[i]
                                                : std::string();

            if (i < seen.painted.size())
            {
                out.digits   = seen.painted[i].digits;
                out.coloured = seen.painted[i].coloured;
                out.plain    = seen.painted[i].plain;
            }

            break;
        }
    }

    return out;
}

}  // namespace gui
}  // namespace bitwisedom
