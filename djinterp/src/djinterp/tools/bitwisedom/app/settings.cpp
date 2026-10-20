/*******************************************************************************
* bitwisedom [app]                                                  settings.cpp
*
* The settings registries, and settings files.
*   Each entry is made by a small factory per kind, from a function that
* reaches the field in a sheet -- or, for the window's own, in the shell.
* One reach serves both directions: reading takes the field, writing
* assigns it, so the two cannot disagree on which field a path names.
*   A settings file is JSON: its format and version, then the window's own
* settings and a sheet's preferences, each an object of values by path --
* a flag as a boolean, a choice or an integer as a number, a real as a
* number, a colour as #RRGGBBAA and text as a string. Reading it gives back
* the payloads a "set" of each would post, so a file is applied exactly as
* the controls would apply it.
*
* path:      /src/djinterp/tools/bitwisedom/app/settings.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./settings.hpp"  // corresponding header

// std
#include <algorithm>  // std::clamp
#include <cmath>      // std::lround
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/codegen.hpp>  // standard_name
#include <djinterp/tools/bitwisedom/json.hpp>     // json_value


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::option_set;

// flag
//   a setting over a bool.
template<typename Reach>
setting
flag(
    const char* _path,
    const char* _label,
    Reach       _reach,
    const char* _help = ""
)
{
    setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::flag;
    made.help  = _help;
    made.read  = [_reach](const sheet& _sheet)
    {
        return _reach(const_cast<sheet&>(_sheet)) ? 1.0 : 0.0;
    };
    made.write = [_reach](sheet& _sheet, const option_set& _payload)
    {
        bool& field = _reach(_sheet);
        field = _payload.as_bool("value", field);
    };

    return made;
}

// integer
//   a setting over an int, held to its bounds.
template<typename Reach>
setting
integer(
    const char* _path,
    const char* _label,
    int         _low,
    int         _high,
    Reach       _reach
)
{
    setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::integer;
    made.low   = _low;
    made.high  = _high;
    made.read  = [_reach](const sheet& _sheet)
    {
        return static_cast<double>(_reach(const_cast<sheet&>(_sheet)));
    };
    made.write = [_reach, _low, _high](sheet& _sheet,
                                       const option_set& _payload)
    {
        int&       field = _reach(_sheet);
        const long value = _payload.as_long("value", field);

        field = static_cast<int>(std::clamp<long>(value, _low, _high));
    };

    return made;
}

// real
//   a setting over a float, held to its bounds.
template<typename Reach>
setting
real(
    const char* _path,
    const char* _label,
    double      _low,
    double      _high,
    Reach       _reach,
    const char* _help = ""
)
{
    setting made;
    made.path  = _path;
    made.label = _label;
    made.help  = _help;
    made.kind  = setting_kind::real;
    made.low   = _low;
    made.high  = _high;
    made.read  = [_reach](const sheet& _sheet)
    {
        return static_cast<double>(_reach(const_cast<sheet&>(_sheet)));
    };
    made.write = [_reach, _low, _high](sheet& _sheet,
                                       const option_set& _payload)
    {
        float&       field = _reach(_sheet);
        const double value = _payload.as_double("value", field);

        field = static_cast<float>(std::clamp(value, _low, _high));
    };

    return made;
}

// colour
//   a setting over a packed colour.
template<typename Reach>
setting
colour(
    const char* _path,
    const char* _label,
    Reach       _reach
)
{
    setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::colour;
    made.read  = [_reach](const sheet& _sheet)
    {
        return static_cast<double>(_reach(const_cast<sheet&>(_sheet)));
    };
    made.write = [_reach](sheet& _sheet, const option_set& _payload)
    {
        rgba& field = _reach(_sheet);

        field = make_rgba(
            static_cast<float>(_payload.as_double("r", channel(field, 0))),
            static_cast<float>(_payload.as_double("g", channel(field, 1))),
            static_cast<float>(_payload.as_double("b", channel(field, 2))),
            static_cast<float>(_payload.as_double("a", channel(field, 3))));
    };

    return made;
}

// choice
//   a setting over an enumeration whose first choice is _first.
template<typename Enum,
         typename Reach>
setting
choice(
    const char*              _path,
    const char*              _label,
    std::vector<std::string> _choices,
    Reach                    _reach,
    int                      _first = 0
)
{
    setting made;
    made.path    = _path;
    made.label   = _label;
    made.kind    = setting_kind::choice;
    made.high    = static_cast<double>(_choices.size()) - 1.0;
    made.choices = std::move(_choices);
    made.read    = [_reach, _first](const sheet& _sheet)
    {
        return static_cast<double>(
            static_cast<int>(_reach(const_cast<sheet&>(_sheet))) - _first);
    };
    made.write = [_reach, _first, count = made.choices.size()](
                     sheet&            _sheet,
                     const option_set& _payload)
    {
        const long index = std::clamp<long>(_payload.as_long("value", 0),
                                            0,
                                            static_cast<long>(count) - 1);

        _reach(_sheet) = static_cast<Enum>(static_cast<int>(index) + _first);
    };

    return made;
}

// text
//   a setting over a string.
template<typename Reach>
setting
text(
    const char* _path,
    const char* _label,
    Reach       _reach
)
{
    setting made;
    made.path      = _path;
    made.label     = _label;
    made.kind      = setting_kind::text;
    made.read      = [](const sheet&) { return 0.0; };
    made.read_text = [_reach](const sheet& _sheet)
    {
        return _reach(const_cast<sheet&>(_sheet));
    };
    made.write = [_reach](sheet& _sheet, const option_set& _payload)
    {
        std::string& field = _reach(_sheet);
        field = _payload.as_string("value", field);
    };

    return made;
}

// line
//   the four settings of one kind of grid line.
void
line(
    std::vector<setting>&   _out,
    const std::string&      _key,
    line_spec grid_settings::* _member
)
{
    static std::vector<std::string> paths;

    // the paths must outlive the settings that point at them
    paths.push_back("grid." + _key + ".show");
    paths.push_back("grid." + _key + ".thickness");
    paths.push_back("grid." + _key + ".colour");
    paths.push_back("grid." + _key + ".style");

    const std::size_t at = paths.size() - 4;

    _out.push_back(flag(paths[at].c_str(), "show",
                        [_member](sheet& _s) -> bool&
                        {
                            return (_s.grid.*_member).show;
                        }));
    _out.push_back(real(paths[at + 1].c_str(), "thickness", 0.5, 6.0,
                        [_member](sheet& _s) -> float&
                        {
                            return (_s.grid.*_member).thickness;
                        }));
    _out.push_back(colour(paths[at + 2].c_str(), "colour",
                          [_member](sheet& _s) -> rgba&
                          {
                              return (_s.grid.*_member).colour;
                          }));
    _out.push_back(choice<line_style>(
        paths[at + 3].c_str(), "style", { "solid", "dashed", "dotted" },
        [_member](sheet& _s) -> line_style&
        {
            return (_s.grid.*_member).style;
        }));

    return;
}

std::vector<setting>
make_settings()
{
    std::vector<setting> out;

    // ------------------------------------------------------------- the sheet
    out.push_back(text("sheet.title", "Title",
                       [](sheet& _s) -> std::string& { return _s.title; }));
    out.push_back(integer("sheet.width", "Width (bits)", 1, MAX_WIDTH,
                          [](sheet& _s) -> int& { return _s.width; }));
    out.push_back(choice<numbering>(
        "sheet.numbering", "Bit numbering", { "LSB 0", "MSB 0" },
        [](sheet& _s) -> numbering& { return _s.bits; }));
    out.push_back(flag("sheet.msb_left", "MSB on the left",
                       [](sheet& _s) -> bool& { return _s.msb_left; }));
    out.push_back(choice<byte_order>(
        "sheet.bytes", "Byte numbering", { "byte 0 lowest", "byte 0 highest" },
        [](sheet& _s) -> byte_order& { return _s.bytes; }));
    out.push_back(text("sheet.prefix", "Name prefix",
                       [](sheet& _s) -> std::string& { return _s.prefix; }));
    out.push_back(text("sheet.suffix", "Name suffix",
                       [](sheet& _s) -> std::string& { return _s.suffix; }));
    out.push_back(text("sheet.trim_both", "Both ends",
                       [](sheet& _s) -> std::string& { return _s.trim_both; }));
    out.push_back(text("sheet.trim_left", "Left only",
                       [](sheet& _s) -> std::string& { return _s.trim_left; }));
    out.push_back(text("sheet.trim_right", "Right only",
                       [](sheet& _s) -> std::string& { return _s.trim_right; }));
    out.push_back(text("sheet.environment", "Environment",
                       [](sheet& _s) -> std::string&
                       {
                           return _s.environment;
                       }));

    // ----------------------------------------------------------- grid lines
    out.push_back(flag("grid.lines", "Grid lines",
                       [](sheet& _s) -> bool& { return _s.grid.lines; },
                       "every grid line at once"));
    out.push_back(flag("grid.lines_past_rows", "Lines past the last row",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.lines_past_rows;
                       },
                       "the bits' lines run on below the last row, down "
                       "the room above the rows pinned at the bottom; off, "
                       "they stop where the rows do"));
    line(out, "minor", &grid_settings::minor);
    line(out, "major", &grid_settings::major);
    line(out, "rows", &grid_settings::rows);
    line(out, "groups", &grid_settings::groups);
    line(out, "segments", &grid_settings::segments);
    line(out, "border", &grid_settings::border);
    out.push_back(integer("grid.major_every", "Major line every", 2, 32,
                          [](sheet& _s) -> int&
                          {
                              return _s.grid.major_every;
                          }));

    // ---------------------------------------------------------------- cells
    out.push_back(flag("grid.square_cells", "Proportional cells",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.square_cells;
                       },
                       "every row as tall as a bit's cell is wide, times "
                       "the share below -- 100% for square cells: the "
                       "rows follow the cells' width, and the cell height "
                       "is not used"));
    out.push_back(real("grid.cell_ratio", "Row height, of cell width", 0.4,
                       1.5,
                       [](sheet& _s) -> float& { return _s.grid.cell_ratio; },
                       "with proportional cells, how tall a row is for a "
                       "bit's cell's width: 1.0 is square, 0.8 a fifth "
                       "shorter"));
    out.push_back(real("grid.cell_width", "Cell width", 8.0, 56.0,
                       [](sheet& _s) -> float& { return _s.grid.cell_width; },
                       "a bit's cell's width; set by hand, it is the width "
                       "the cells take, and the room to spare goes to "
                       "nothing"));
    out.push_back(real("grid.cell_height", "Cell height", 12.0, 56.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.grid.cell_height;
                       }));
    out.push_back(real("grid.cell_gap", "Cell gap", 0.0, 8.0,
                       [](sheet& _s) -> float& { return _s.grid.cell_gap; }));
    out.push_back(real("grid.rounding", "Rounding", 0.0, 12.0,
                       [](sheet& _s) -> float& { return _s.grid.rounding; }));
    out.push_back(real("grid.name_width", "Name column", 80.0, 1000.0,
                       [](sheet& _s) -> float& { return _s.grid.name_width; }));
    out.push_back(real("grid.value_width", "Value column", 0.0, 600.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.grid.value_width;
                       }));
    out.push_back(real("grid.indent", "Indent", 0.0, 40.0,
                       [](sheet& _s) -> float& { return _s.grid.indent; }));

    // ----------------------------------------------------- columns, labels
    setting segments;
    segments.path    = "grid.auto_segments";
    segments.label   = "Automatic segments";
    segments.kind    = setting_kind::choice;
    segments.choices = { "none", "nibbles", "bytes", "half-words" };
    segments.high    = 3.0;
    segments.read    = [](const sheet& _s)
    {
        return (_s.grid.auto_segments == 4)  ? 1.0
             : (_s.grid.auto_segments == 8)  ? 2.0
             : (_s.grid.auto_segments == 16) ? 3.0
                                             : 0.0;
    };
    segments.write = [](sheet& _s, const option_set& _payload)
    {
        const long index = _payload.as_long("value", 2);
        _s.grid.auto_segments = (index == 1) ? 4
                              : (index == 2) ? 8
                              : (index == 3) ? 16
                                             : 0;
    };
    out.push_back(std::move(segments));

    out.push_back(integer("grid.index_every", "Label every n bits", 1, 16,
                          [](sheet& _s) -> int&
                          {
                              return _s.grid.index_every;
                          }));
    out.push_back(flag("grid.show_indices", "Bit indices",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_indices;
                       }));
    out.push_back(flag("grid.show_digits", "Digits in cells",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_digits;
                       }));
    out.push_back(flag("grid.show_names", "Symbols column",
                       [](sheet& _s) -> bool&
                       { return _s.grid.show_names; }));
    out.push_back(flag("grid.show_values", "Value column",
                       [](sheet& _s) -> bool&
                       { return _s.grid.show_values; }));
    out.push_back(flag("grid.show_bits", "Bit/byte visualizer",
                       [](sheet& _s) -> bool&
                       { return _s.grid.show_bits; }));
    out.push_back(flag("grid.values_first", "Value before Symbols",
                       [](sheet& _s) -> bool&
                       { return _s.grid.values_first; }));
    out.push_back(flag("grid.show_labels", "Names across ranges",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_labels;
                       }));
    out.push_back(flag("grid.show_colours", "Colours in cells",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_colours;
                       },
                       "the rows' colours in their cells: off, their bits "
                       "are drawn plain -- the grid's display rules may "
                       "show them in places"));
    out.push_back(flag("grid.show_cell_boxes", "Cell boxes",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_cell_boxes;
                       },
                       "draw each cell as its own rounded box"));
    out.push_back(flag("grid.show_probe", "Probe row",
                       [](sheet& _s) -> bool& { return _s.grid.show_probe; }));
    out.push_back(flag("grid.show_usage", "Usage row",
                       [](sheet& _s) -> bool& { return _s.grid.show_usage; }));
    out.push_back(flag("grid.full_names", "Macro names in the grid",
                       [](sheet& _s) -> bool& { return _s.grid.full_names; }));

    // what the symbols column writes for each row -- the buttons over it,
    // and their menu: its symbol name, its own name, or its value, written
    // as the header writes it or, lined up, every row alike. Values leave
    // the macro names alone, which still say how a range's name is written
    // across its cells; one choice is one setting, and one step to undo.
    setting names_by;
    names_by.path    = "grid.names_by";
    names_by.label   = "Symbols by";
    names_by.kind    = setting_kind::choice;
    names_by.choices = { "symbol name", "name", "value, as written",
                         "value in hex", "value in decimal",
                         "value in binary", "value as a shifted run",
                         "value as the bits it sets" };
    names_by.high    = static_cast<double>(NAMES_BY_VALUE +
                                           VALUE_FORM_COUNT - 1);
    names_by.help    = "what the symbols column writes for each row: its "
                       "symbol name, its own name, or its value -- as the "
                       "header writes it, or every row alike, lined up";
    names_by.read    = [](const sheet& _s)
    {
        return (_s.grid.names_by_value)
                   ? static_cast<double>(NAMES_BY_VALUE +
                                         static_cast<int>(
                                             _s.grid.names_value_form))
             : (_s.grid.full_names) ? 0.0
                                    : 1.0;
    };
    names_by.write = [](sheet& _s, const option_set& _payload)
    {
        const long index = std::clamp<long>(
            _payload.as_long("value", 1),
            0,
            NAMES_BY_VALUE + VALUE_FORM_COUNT - 1);

        _s.grid.names_by_value = (index >= NAMES_BY_VALUE);

        // by value, the form it is written in; else the macro names, which
        // values leave as they were
        if (index >= NAMES_BY_VALUE)
        {
            _s.grid.names_value_form =
                static_cast<value_form>(index - NAMES_BY_VALUE);
        }
        else
        {
            _s.grid.full_names = (index == 0);
        }
    };
    out.push_back(std::move(names_by));

    // the order the rows are shown in: the sheet's own, or each group's
    // sorted -- the grid and the tree alike, the header never
    setting sort_by = choice<sort_key>(
        "grid.sort_by", "Sort the rows by",
        { "the sheet's own order", "symbol name", "name", "short name",
          "value", "lowest bit", "width", "kind", "note" },
        [](sheet& _s) -> sort_key& { return _s.grid.sort_by; });
    sort_by.help = "the order the grid and the tree show each group's rows "
                   "in: the sheet's own, or sorted -- groups first, a row "
                   "the key says nothing of last. The header keeps the "
                   "sheet's order";
    out.push_back(std::move(sort_by));
    out.push_back(flag("grid.sort_descending", "Sort descending",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.sort_descending;
                       },
                       "sorted from the last to the first: Z to A, the "
                       "largest value first"));

    out.push_back(flag("grid.abbreviate_unused", "Abbreviate unused runs",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.abbreviate_unused;
                       },
                       "runs of bits no visible row uses shrink to one "
                       "narrow column"));
    out.push_back(integer("grid.unused_run", "Shortest run abbreviated", 2,
                          32,
                          [](sheet& _s) -> int&
                          {
                              return _s.grid.unused_run;
                          }));
    out.push_back(choice<collapsed_style>(
        "grid.collapsed", "A collapsed segment shows",
        { "its bits in hex", "a dot where set", "how many are set",
          "nothing" },
        [](sheet& _s) -> collapsed_style& { return _s.grid.collapsed; }));

    setting fill = choice<grid_fill>(
        "grid.fill", "Room to spare goes to",
        { "the bits' cells", "the name column", "nothing" },
        [](sheet& _s) -> grid_fill& { return _s.grid.fill; });
    fill.help = "when the grid is wider than its columns need: the cells "
                "widen alike, or the name column takes it all, or the grid "
                "ends where its columns do";
    out.push_back(std::move(fill));

    // -------------------------------------------------------------- colours
    // the theme is the window's (the Theme menu); the palette, the scheme's
    // unless the sheet chooses one
    std::vector<std::string> palettes = { "the scheme's" };

    for (int i = 0; i < SCHEME_COUNT; ++i)
    {
        palettes.emplace_back(scheme_name(i));
    }

    setting palette = choice<int>(
        "colours.palette", "Palette", palettes,
        [](sheet& _s) -> int& { return _s.colours.palette; },
        -1);
    palette.help = "the colours rows are given when they have none of their "
                   "own; the scheme's follows the Theme menu";
    out.push_back(std::move(palette));
    out.push_back(real("colours.saturation", "Saturation", 0.0, 1.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.colours.saturation;
                       }));
    out.push_back(real("colours.value", "Brightness", 0.1, 1.0,
                       [](sheet& _s) -> float& { return _s.colours.value; }));
    out.push_back(choice<colour_mode>(
        "colours.mode", "Colour set bits by",
        { "their row", "their group", "their kind", "the accent" },
        [](sheet& _s) -> colour_mode& { return _s.colours.mode; }));
    out.push_back(real("colours.set_alpha", "Set-bit opacity", 0.1, 1.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.colours.set_alpha;
                       }));
    out.push_back(real("colours.range_alpha", "Range tint", 0.0, 1.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.colours.range_alpha;
                       }));
    out.push_back(colour("colours.accent", "Accent",
                         [](sheet& _s) -> rgba& { return _s.colours.accent; }));
    out.push_back(colour("colours.empty_cell", "Empty cell",
                         [](sheet& _s) -> rgba&
                         {
                             return _s.colours.empty_cell;
                         }));
    out.push_back(colour("colours.collision", "Collision",
                         [](sheet& _s) -> rgba&
                         {
                             return _s.colours.collision;
                         }));
    out.push_back(colour("colours.probe", "Probe",
                         [](sheet& _s) -> rgba& { return _s.colours.probe; }));
    out.push_back(colour("colours.selection", "Selection",
                         [](sheet& _s) -> rgba&
                         {
                             return _s.colours.selection;
                         }));
    out.push_back(colour("colours.hover", "Hover",
                         [](sheet& _s) -> rgba& { return _s.colours.hover; }));
    out.push_back(colour("colours.ghost", "Preview outline",
                         [](sheet& _s) -> rgba& { return _s.colours.ghost; }));

    // ------------------------------------------------------------ generator
    std::vector<std::string> levels;

    for (int i = 0; i < STANDARD_COUNT; ++i)
    {
        levels.emplace_back(standard_name(static_cast<standard>(i)));
    }

    setting level = choice<standard>(
        "gen.level", "Language level", levels,
        [](sheet& _s) -> standard& { return _s.gen.level; });
    level.write = [](sheet& _s, const option_set& _payload)
    {
        const long index = std::clamp<long>(_payload.as_long("value", 0),
                                            0,
                                            STANDARD_COUNT - 1);

        // the level decides the language
        _s.gen.level = static_cast<standard>(index);
        _s.gen.lang  = is_cpp(_s.gen.level) ? language::cpp : language::c;
    };
    out.push_back(std::move(level));

    out.push_back(choice<gen_form>(
        "gen.form", "Form", { "macros", "C++ constants" },
        [](sheet& _s) -> gen_form& { return _s.gen.form; }));
    out.push_back(choice<gen_style>(
        "gen.style", "Style", { "djinterp", "plain" },
        [](sheet& _s) -> gen_style& { return _s.gen.style; }));
    out.push_back(choice<radix>(
        "gen.base", "Literals in", { "hex", "decimal", "octal", "binary",
                                     "shifts" },
        [](sheet& _s) -> radix& { return _s.gen.base; }));
    out.push_back(choice<int_suffix>(
        "gen.suffix", "Integer suffix", { "none", "u", "ul", "ull" },
        [](sheet& _s) -> int_suffix& { return _s.gen.suffix; }));
    out.push_back(flag("gen.upper_hex", "Upper-case hex",
                       [](sheet& _s) -> bool& { return _s.gen.upper_hex; }));
    out.push_back(flag("gen.upper_suffix", "Upper-case suffix",
                       [](sheet& _s) -> bool&
                       {
                           return _s.gen.upper_suffix;
                       }));
    out.push_back(flag("gen.pad", "Pad to the width",
                       [](sheet& _s) -> bool& { return _s.gen.pad; }));
    out.push_back(flag("gen.banner", "Banner",
                       [](sheet& _s) -> bool& { return _s.gen.banner; }));
    out.push_back(flag("gen.guard", "Include guard",
                       [](sheet& _s) -> bool& { return _s.gen.guard; }));
    out.push_back(flag("gen.pragma_once", "#pragma once instead",
                       [](sheet& _s) -> bool& { return _s.gen.pragma_once; }));
    out.push_back(flag("gen.comments", "Briefs",
                       [](sheet& _s) -> bool& { return _s.gen.comments; }));
    out.push_back(flag("gen.sections", "Numbered sections",
                       [](sheet& _s) -> bool& { return _s.gen.sections; }));
    out.push_back(flag("gen.align", "Align values",
                       [](sheet& _s) -> bool& { return _s.gen.align; }));
    out.push_back(text("gen.file_name", "File name",
                       [](sheet& _s) -> std::string&
                       {
                           return _s.gen.file_name;
                       }));
    out.push_back(text("gen.guard_name", "Guard (empty: derived)",
                       [](sheet& _s) -> std::string&
                       {
                           return _s.gen.guard_name;
                       }));
    out.push_back(text("gen.name_space", "C++ namespace",
                       [](sheet& _s) -> std::string&
                       {
                           return _s.gen.name_space;
                       }));
    out.push_back(text("gen.author", "Author",
                       [](sheet& _s) -> std::string&
                       {
                           return _s.gen.author;
                       }));

    // a wide sheet's arrays: words of 8, 16, 32 or 64 bits
    setting word;
    word.path    = "gen.word";
    word.label   = "Array words";
    word.kind    = setting_kind::choice;
    word.choices = { "8 bits", "16 bits", "32 bits", "64 bits" };
    word.high    = 3.0;
    word.help    = "a sheet wider than an integer is written as an array of "
                   "words this wide";
    word.read    = [](const sheet& _s)
    {
        switch (word_bits(_s))
        {
            case 8:  return 0.0;
            case 16: return 1.0;
            case 64: return 3.0;
            default: return 2.0;
        }
    };
    word.write   = [](sheet& _s, const option_set& _payload)
    {
        const long index = std::clamp<long>(_payload.as_long("value", 2),
                                            0,
                                            3);

        _s.gen.word = 8 << index;
    };
    out.push_back(std::move(word));

    setting model = choice<data_model>(
        "gen.model", "Data model",
        { "LP64 (Linux, macOS)", "LLP64 (Windows)", "ILP32 (32-bit)" },
        [](sheet& _s) -> data_model& { return _s.gen.model; });
    model.help = "the widths C leaves to the target -- long's, a pointer's "
                 "-- as macros are evaluated";
    out.push_back(std::move(model));

    return out;
}

}  // anonymous namespace


const std::vector<setting>&
settings()
{
    static const std::vector<setting> table = make_settings();

    return table;
}

const setting*
find_setting(
    const std::string& _path
)
{
    for (const setting& each : settings())
    {
        if (each.path == _path)
        {
            return &each;
        }
    }

    return nullptr;
}

// ------------------------------------------------ the window's own settings

namespace
{

// ADD_KIND_NAMES / ADD_KIND_OF
//   constants: each kind a row may be added as, in ADD_KINDS's order: its
// name, and the kind "edit.add" takes for it.
const char* const ADD_KIND_NAMES[ADD_KINDS] = {
    "Bit", "Range", "Value in a range", "Literal", "Expression", "Macro",
    "Group" };
const int ADD_KIND_OF[ADD_KINDS] = { 0, 1, 2, 3, 4, 5, -1 };

// env_flag
//   one of the window's settings over a bool.
template<typename Reach>
env_setting
env_flag(
    const char* _path,
    const char* _label,
    const char* _help,
    Reach       _reach
)
{
    env_setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::flag;
    made.help  = _help;
    made.read  = [_reach](const shell_state& _shell)
    {
        return _reach(const_cast<shell_state&>(_shell)) ? 1.0 : 0.0;
    };
    made.write = [_reach](shell_state& _shell, const option_set& _payload)
    {
        bool& field = _reach(_shell);
        field = _payload.as_bool("value", field);
    };

    return made;
}

// env_choice
//   one of the window's settings over a small whole number, held to its
// choices: _get reads it as the choice's index, and _set writes one.
template<typename Get,
         typename Set>
env_setting
env_choice(
    const char*              _path,
    const char*              _label,
    std::vector<std::string> _choices,
    const char*              _help,
    Get                      _get,
    Set                      _set
)
{
    env_setting made;
    made.path    = _path;
    made.label   = _label;
    made.kind    = setting_kind::choice;
    made.high    = static_cast<double>(_choices.size()) - 1.0;
    made.choices = std::move(_choices);
    made.help    = _help;
    made.read    = [_get](const shell_state& _shell)
    {
        return static_cast<double>(_get(_shell));
    };
    made.write   = [_set, count = made.choices.size()](
                       shell_state&      _shell,
                       const option_set& _payload)
    {
        const long index = std::clamp<long>(_payload.as_long("value", 0),
                                            0,
                                            static_cast<long>(count) - 1);

        _set(_shell, static_cast<int>(index));
    };

    return made;
}

std::vector<env_setting>
make_environment()
{
    std::vector<env_setting> out;
    std::vector<std::string> themes;
    std::vector<std::string> schemes;
    std::vector<std::string> kinds;

    for (int i = 0; i < THEME_COUNT; ++i)
    {
        themes.emplace_back(theme_name(i));
    }

    for (int i = 0; i < SCHEME_COUNT; ++i)
    {
        schemes.emplace_back(scheme_name(i));
    }

    for (const char* each : ADD_KIND_NAMES)
    {
        kinds.emplace_back(each);
    }

    out.push_back(env_choice(
        "environment.theme", "Theme", themes,
        "the window's theme: every sheet is drawn in it",
        [](const shell_state& _s) { return _s.theme; },
        [](shell_state& _s, int _index) { _s.theme = _index; }));
    out.push_back(env_choice(
        "environment.scheme", "Colour scheme", schemes,
        "the window's colours, and the palette rows take theirs from "
        "when they leave it to the scheme",
        [](const shell_state& _s) { return _s.scheme; },
        [](shell_state& _s, int _index) { _s.scheme = _index; }));
    out.push_back(env_flag(
        "environment.restore", "Open where I left off",
        "the window opens as it was closed: its workspaces, every sheet "
        "in them -- unsaved changes too -- and how they were laid out",
        [](shell_state& _s) -> bool& { return _s.prefs.restore; }));
    out.push_back(env_choice(
        "environment.merged", "The symbols tree",
        { "in the grid's symbols column", "in a window of its own" },
        "the grid's symbols column is the tree: its groups open and close, "
        "and its rows drag and take children there. Its own window shows "
        "the tree beside the grid as well",
        [](const shell_state& _s) { return _s.prefs.merged ? 0 : 1; },
        [](shell_state& _s, int _index) { _s.prefs.merged = (_index == 0); }));
    out.push_back(env_choice(
        "environment.add", "Add rows with",
        { "one button, and a menu of what to add",
          "a split button: what it shows, or another kind" },
        "how the window offers to add a row: a button whose menu lists "
        "what to add, or a button that adds the kind it shows beside an "
        "arrow that chooses another",
        [](const shell_state& _s) { return static_cast<int>(_s.prefs.add); },
        [](shell_state& _s, int _index)
        {
            _s.prefs.add = static_cast<add_style>(_index);
        }));
    out.push_back(env_choice(
        "environment.add_kind", "The split button adds", kinds,
        "what the split button adds until its arrow chooses another kind",
        [](const shell_state& _s) { return _s.prefs.add_kind; },
        [](shell_state& _s, int _index) { _s.prefs.add_kind = _index; }));
    out.push_back(env_choice(
        "environment.child", "Under a group's rows",
        { "the Add child field, and a + for the Add node window",
          "the Add child field",
          "a + for the Add node window" },
        "what ends a group's or a range's rows, to add one more: a field "
        "where a name typed is a bit, and a + that opens the Add node "
        "window, where any kind of row is made in full",
        [](const shell_state& _s) { return static_cast<int>(_s.prefs.child); },
        [](shell_state& _s, int _index)
        {
            _s.prefs.child = static_cast<child_offer>(_index);
        }));
    out.push_back(env_flag(
        "environment.child_fill", "The Add child field fills its column",
        "the field reaches the column's right edge; off, it is as wide as "
        "what it holds",
        [](shell_state& _s) -> bool& { return _s.prefs.child_fill; }));
    out.push_back(env_choice(
        "environment.node_as", "The Add node window opens",
        { "as a tab, among the sheets", "as a window of its own" },
        "where the + under a group's rows, and Edit > Add node, open the "
        "Add node window",
        [](const shell_state& _s)
        {
            return static_cast<int>(_s.prefs.node_as);
        },
        [](shell_state& _s, int _index)
        {
            _s.prefs.node_as = static_cast<node_window>(_index);
        }));
    out.push_back(env_choice(
        "environment.new_colour", "New rows' colour",
        { "none: their bits drawn plain",
          "each its own, from the palette",
          "the colour below, every one alike" },
        "the colour a row takes when it is added, until the inspector "
        "gives it another",
        [](const shell_state& _s)
        {
            return static_cast<int>(_s.prefs.colour);
        },
        [](shell_state& _s, int _index)
        {
            _s.prefs.colour = static_cast<new_colour>(_index);
        }));

    env_setting rgba_setting;
    rgba_setting.path  = "environment.new_rgba";
    rgba_setting.label = "Colour for new rows";
    rgba_setting.kind  = setting_kind::colour;
    rgba_setting.help  = "the colour every new row takes, when new rows "
                         "take the colour below";
    rgba_setting.read  = [](const shell_state& _s)
    {
        return static_cast<double>(_s.prefs.colour_rgba);
    };
    rgba_setting.write = [](shell_state& _s, const option_set& _payload)
    {
        rgba& field = _s.prefs.colour_rgba;

        field = make_rgba(
            static_cast<float>(_payload.as_double("r", channel(field, 0))),
            static_cast<float>(_payload.as_double("g", channel(field, 1))),
            static_cast<float>(_payload.as_double("b", channel(field, 2))),
            static_cast<float>(_payload.as_double("a", channel(field, 3))));

        // a colour with no alpha would be no colour at all
        if (channel(field, 3) <= 0.0f)
        {
            field = with_alpha(field, 1.0f);
        }
    };
    out.push_back(std::move(rgba_setting));

    return out;
}

// colour_payload
//   a packed colour as the payload a color_field posts.
option_set
colour_payload(
    rgba _colour
)
{
    option_set made;

    made.set("r", static_cast<double>(channel(_colour, 0)));
    made.set("g", static_cast<double>(channel(_colour, 1)));
    made.set("b", static_cast<double>(channel(_colour, 2)));
    made.set("a", static_cast<double>(channel(_colour, 3)));

    return made;
}

// json_of
//   a setting's value, read as a number, as a settings file writes it.
json_value
json_of(
    setting_kind _kind,
    double       _value
)
{
    switch (_kind)
    {
        case setting_kind::flag:
            return json_value::boolean(_value != 0.0);

        case setting_kind::integer:
        case setting_kind::choice:
            return json_value::number(static_cast<i64>(std::lround(_value)));

        case setting_kind::real:
            return json_value::real(_value);

        case setting_kind::colour:
            return json_value::string(rgba_text(static_cast<rgba>(_value)));

        case setting_kind::text:
            break;
    }

    return json_value();
}

// payload_of
//   a value a settings file holds as the payload a "set" of a setting of
// that kind posts; false for a value of another kind.
bool
payload_of(
    setting_kind      _kind,
    const json_value& _value,
    option_set&       _payload
)
{
    switch (_kind)
    {
        case setting_kind::flag:
            if (_value.type != json_type::boolean)
            {
                return false;
            }

            _payload.set("value", _value.flag);

            return true;

        case setting_kind::integer:
        case setting_kind::choice:
            if (_value.type != json_type::number)
            {
                return false;
            }

            _payload.set("value", static_cast<long>(_value.as_int(0)));

            return true;

        case setting_kind::real:
            if (_value.type != json_type::number)
            {
                return false;
            }

            _payload.set("value", _value.as_real(0.0));

            return true;

        case setting_kind::colour:
        {
            rgba colour = 0;

            if ( (_value.type != json_type::string) ||
                 (!parse_rgba(_value.as_string(""), colour)) )
            {
                return false;
            }

            _payload = colour_payload(colour);

            return true;
        }

        case setting_kind::text:
            if (_value.type != json_type::string)
            {
                return false;
            }

            _payload.set("value", _value.as_string(""));

            return true;
    }

    return false;
}

}  // anonymous namespace


const char*
add_kind_name(
    int _index
)
{
    return ADD_KIND_NAMES[std::clamp(_index, 0, ADD_KINDS - 1)];
}

int
add_kind_of(
    int _index
)
{
    return ADD_KIND_OF[std::clamp(_index, 0, ADD_KINDS - 1)];
}

const std::vector<env_setting>&
environment_settings()
{
    static const std::vector<env_setting> table = make_environment();

    return table;
}

const env_setting*
find_env_setting(
    const std::string& _path
)
{
    for (const env_setting& each : environment_settings())
    {
        if (each.path == _path)
        {
            return &each;
        }
    }

    return nullptr;
}

bool
is_environment(
    const std::string& _path
)
{
    const std::string head = ENVIRONMENT;

    return (_path.compare(0, head.size(), head) == 0);
}

bool
is_preference(
    const setting& _setting
)
{
    // CONTENT
    //   constant: the sheet's settings that are its content, not how it is
    // drawn or written: they stay with the sheet.
    static const char* const CONTENT[] = {
        "sheet.title", "sheet.width", "sheet.prefix", "sheet.suffix",
        "sheet.trim_both", "sheet.trim_left", "sheet.trim_right",
        "sheet.environment", "gen.file_name", "gen.guard_name",
        "gen.name_space" };

    for (const char* each : CONTENT)
    {
        if (_setting.path == each)
        {
            return false;
        }
    }

    return true;
}

std::string
write_settings(
    const shell_state& _shell,
    const sheet&       _sheet
)
{
    json_value root        = json_value::object();
    json_value environment = json_value::object();
    json_value own         = json_value::object();

    root.set("format", json_value::string(SETTINGS_FORMAT));
    root.set("version", json_value::number(SETTINGS_VERSION));

    for (const env_setting& each : environment_settings())
    {
        environment.set(each.path, json_of(each.kind, each.read(_shell)));
    }

    for (const setting& each : settings())
    {
        if (!is_preference(each))
        {
            continue;
        }

        own.set(each.path,
                (each.kind == setting_kind::text)
                    ? json_value::string(each.read_text(_sheet))
                    : json_of(each.kind, each.read(_sheet)));
    }

    root.set("environment", std::move(environment));
    root.set("sheet", std::move(own));

    return write_json(root) + "\n";
}

bool
read_settings(
    const std::string&          _text,
    std::vector<setting_value>& _values,
    std::string&                _error
)
{
    json_value root;

    _values.clear();

    if (!parse_json(_text, root, _error))
    {
        return false;
    }

    if ( (root.type != json_type::object) ||
         (root.find("format") == nullptr) ||
         (root.find("format")->as_string("") != SETTINGS_FORMAT) )
    {
        _error = "not a bitwisedom settings file";

        return false;
    }

    const json_value* version = root.find("version");

    if ( (version != nullptr) &&
         (version->as_int(0) > SETTINGS_VERSION) )
    {
        _error = "a settings file from a newer bitwisedom";

        return false;
    }

    const json_value* environment = root.find("environment");
    const json_value* own         = root.find("sheet");

    if ( (environment != nullptr) &&
         (environment->type == json_type::object) )
    {
        for (const json_member& each : environment->members)
        {
            const env_setting* found = find_env_setting(each.key);
            setting_value      made;

            made.path = each.key;

            if ( (found != nullptr) &&
                 (payload_of(found->kind, each.value, made.payload)) )
            {
                _values.push_back(std::move(made));
            }
        }
    }

    if ( (own != nullptr) &&
         (own->type == json_type::object) )
    {
        for (const json_member& each : own->members)
        {
            const setting* found = find_setting(each.key);
            setting_value  made;

            made.path = each.key;

            if ( (found != nullptr)        &&
                 (is_preference(*found))   &&
                 (payload_of(found->kind, each.value, made.payload)) )
            {
                _values.push_back(std::move(made));
            }
        }
    }

    return true;
}

}  // namespace gui
}  // namespace bitwisedom
