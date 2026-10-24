/*******************************************************************************
* bitwisedom [bitwisedom]                                           settings.cpp
*
* A sheet's settings, by path.
*   Each entry is made by a small factory per kind, from a function that
* reaches the field in a sheet. One reach serves both directions: reading
* takes the field, writing assigns it, so the two cannot disagree on which
* field a path names. A setting whose value is not one field -- the symbols
* column's, which is three; the array words, which are a power of two --
* has its own read and write.
*   As text, a number is read and written with the C library's
* locale-independent conversions, std::from_chars and std::to_chars, where
* the library has them for reals, and otherwise by strtod in the "C" locale
* a program starts in.
*
* path:      /src/djinterp/tools/bitwisedom/settings.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include <djinterp/tools/bitwisedom/settings.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>     // std::clamp
#include <cctype>        // std::tolower
#include <charconv>      // std::from_chars, std::to_chars
#include <cmath>         // std::lround, std::isfinite
#include <cstdio>        // std::snprintf
#include <cstdlib>       // std::strtod
#include <system_error>  // std::errc
#include <utility>       // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>     // format_dec
#include <djinterp/tools/bitwisedom/codegen.hpp>  // standard_name, is_cpp


namespace bitwisedom
{

namespace
{

// words
//   type: the words of a choice, one for each of its values, in order.
using words = std::vector<std::string>;

// flag
//   a setting over a bool.
template<typename Reach>
sheet_setting
flag(
    const char* _path,
    const char* _label,
    Reach       _reach,
    const char* _help = ""
)
{
    sheet_setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::flag;
    made.help  = _help;
    made.read  = [_reach](const sheet& _sheet)
    {
        return _reach(const_cast<sheet&>(_sheet)) ? 1.0 : 0.0;
    };
    made.write = [_reach](sheet& _sheet, double _value)
    {
        _reach(_sheet) = (_value != 0.0);
    };

    return made;
}

// integer
//   a setting over an int, held to its bounds.
template<typename Reach>
sheet_setting
integer(
    const char* _path,
    const char* _label,
    int         _low,
    int         _high,
    Reach       _reach
)
{
    sheet_setting made;
    made.path  = _path;
    made.label = _label;
    made.kind  = setting_kind::integer;
    made.low   = _low;
    made.high  = _high;
    made.read  = [_reach](const sheet& _sheet)
    {
        return static_cast<double>(_reach(const_cast<sheet&>(_sheet)));
    };
    made.write = [_reach, _low, _high](sheet& _sheet, double _value)
    {
        _reach(_sheet) = static_cast<int>(
            std::clamp<long>(std::lround(_value), _low, _high));
    };

    return made;
}

// real
//   a setting over a float, held to its bounds.
template<typename Reach>
sheet_setting
real(
    const char* _path,
    const char* _label,
    double      _low,
    double      _high,
    Reach       _reach,
    const char* _help = ""
)
{
    sheet_setting made;
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
    made.write = [_reach, _low, _high](sheet& _sheet, double _value)
    {
        _reach(_sheet) = static_cast<float>(std::clamp(_value, _low, _high));
    };

    return made;
}

// theme_real
//   a real the theme and scheme choose while it is negative: as real, but
// a value below its bounds is written as -1, the theme's again.
template<typename Reach>
sheet_setting
theme_real(
    const char* _path,
    const char* _label,
    double      _low,
    double      _high,
    Reach       _reach
)
{
    sheet_setting made = real(_path, _label, _low, _high, _reach);

    made.automatic = true;
    made.write     = [_reach, _low, _high](sheet& _sheet, double _value)
    {
        _reach(_sheet) = (_value < _low)
                         ? -1.0f
                         : static_cast<float>(std::clamp(_value, _low,
                                                         _high));
    };

    return made;
}

// colour
//   a setting over a packed colour, the theme's and scheme's while it is 0.
template<typename Reach>
sheet_setting
colour(
    const char* _path,
    const char* _label,
    Reach       _reach
)
{
    sheet_setting made;
    made.path      = _path;
    made.label     = _label;
    made.kind      = setting_kind::colour;
    made.automatic = true;
    made.read  = [_reach](const sheet& _sheet)
    {
        return static_cast<double>(_reach(const_cast<sheet&>(_sheet)));
    };
    made.write = [_reach](sheet& _sheet, double _value)
    {
        _reach(_sheet) = static_cast<rgba>(
            std::clamp(_value, 0.0, 4294967295.0));
    };

    return made;
}

// choice
//   a setting over an enumeration whose first choice is _first.
template<typename Enum,
         typename Reach>
sheet_setting
choice(
    const char*              _path,
    const char*              _label,
    std::vector<std::string> _choices,
    words                    _words,
    Reach                    _reach,
    int                      _first = 0
)
{
    sheet_setting made;
    made.path    = _path;
    made.label   = _label;
    made.kind    = setting_kind::choice;
    made.high    = static_cast<double>(_choices.size()) - 1.0;
    made.choices = std::move(_choices);
    made.words   = std::move(_words);
    made.read    = [_reach, _first](const sheet& _sheet)
    {
        return static_cast<double>(
            static_cast<int>(_reach(const_cast<sheet&>(_sheet))) - _first);
    };
    made.write = [_reach, _first, count = made.choices.size()](
                     sheet& _sheet,
                     double _value)
    {
        const long index = std::clamp<long>(std::lround(_value),
                                            0,
                                            static_cast<long>(count) - 1);

        _reach(_sheet) = static_cast<Enum>(static_cast<int>(index) + _first);
    };

    return made;
}

// text
//   a setting over a string.
template<typename Reach>
sheet_setting
text(
    const char* _path,
    const char* _label,
    Reach       _reach
)
{
    sheet_setting made;
    made.path       = _path;
    made.label      = _label;
    made.kind       = setting_kind::text;
    made.read       = [](const sheet&) { return 0.0; };
    made.read_text  = [_reach](const sheet& _sheet)
    {
        return _reach(const_cast<sheet&>(_sheet));
    };
    made.write      = [](sheet&, double) {};
    made.write_text = [_reach](sheet& _sheet, const std::string& _value)
    {
        _reach(_sheet) = _value;
    };

    return made;
}

// word_of
//   a label as a word: lower case, each run of anything but letters and
// digits one underscore -- "Colour-blind safe" is colour_blind_safe.
std::string
word_of(
    const std::string& _label
)
{
    std::string out;
    bool        gap = false;

    for (const char each : _label)
    {
        const unsigned char c = static_cast<unsigned char>(each);

        // a letter or a digit, after the gap before it
        if ( ( (c >= 'a') && (c <= 'z') ) ||
             ( (c >= 'A') && (c <= 'Z') ) ||
             ( (c >= '0') && (c <= '9') ) )
        {
            if ( (gap) &&
                 (!out.empty()) )
            {
                out += '_';
            }

            out += static_cast<char>(std::tolower(c));
            gap  = false;
        }
        else
        {
            gap = true;
        }
    }

    return out;
}

// line
//   the four settings of one kind of grid line.
void
line(
    std::vector<sheet_setting>& _out,
    const std::string&          _key,
    line_spec grid_settings::*  _member
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
        { "solid", "dashed", "dotted" },
        [_member](sheet& _s) -> line_style&
        {
            return (_s.grid.*_member).style;
        }));

    return;
}

// sheet_part
//   the sheet's own settings: its names, its width and its numbering.
void
sheet_part(
    std::vector<sheet_setting>& _out
)
{
    _out.push_back(text("sheet.title", "Title",
                        [](sheet& _s) -> std::string& { return _s.title; }));
    _out.push_back(integer("sheet.width", "Width (bits)", 1, MAX_WIDTH,
                           [](sheet& _s) -> int& { return _s.width; }));
    _out.push_back(choice<numbering>(
        "sheet.numbering", "Bit numbering", { "LSB 0", "MSB 0" },
        { "lsb0", "msb0" },
        [](sheet& _s) -> numbering& { return _s.bits; }));
    _out.push_back(flag("sheet.msb_left", "MSB on the left",
                        [](sheet& _s) -> bool& { return _s.msb_left; }));
    _out.push_back(choice<byte_order>(
        "sheet.bytes", "Byte numbering", { "byte 0 lowest", "byte 0 highest" },
        { "little", "big" },
        [](sheet& _s) -> byte_order& { return _s.bytes; }));
    _out.push_back(text("sheet.prefix", "Name prefix",
                        [](sheet& _s) -> std::string& { return _s.prefix; }));
    _out.push_back(text("sheet.suffix", "Name suffix",
                        [](sheet& _s) -> std::string& { return _s.suffix; }));
    _out.push_back(text("sheet.trim_both", "Both ends",
                        [](sheet& _s) -> std::string&
                        {
                            return _s.trim_both;
                        }));
    _out.push_back(text("sheet.trim_left", "Left only",
                        [](sheet& _s) -> std::string&
                        {
                            return _s.trim_left;
                        }));
    _out.push_back(text("sheet.trim_right", "Right only",
                        [](sheet& _s) -> std::string&
                        {
                            return _s.trim_right;
                        }));
    _out.push_back(text("sheet.environment", "Environment",
                        [](sheet& _s) -> std::string&
                        {
                            return _s.environment;
                        }));

    return;
}

// lines_part
//   the grid's lines.
void
lines_part(
    std::vector<sheet_setting>& _out
)
{
    _out.push_back(flag("grid.lines", "Grid lines",
                        [](sheet& _s) -> bool& { return _s.grid.lines; },
                        "every grid line at once"));
    _out.push_back(flag("grid.lines_past_rows", "Lines past the last row",
                        [](sheet& _s) -> bool&
                        {
                            return _s.grid.lines_past_rows;
                        },
                        "the bits' lines run on below the last row, down "
                        "the room above the rows pinned at the bottom; off, "
                        "they stop where the rows do"));
    line(_out, "minor", &grid_settings::minor);
    line(_out, "major", &grid_settings::major);
    line(_out, "rows", &grid_settings::rows);
    line(_out, "groups", &grid_settings::groups);
    line(_out, "segments", &grid_settings::segments);
    line(_out, "border", &grid_settings::border);
    _out.push_back(integer("grid.major_every", "Major line every", 2, 32,
                           [](sheet& _s) -> int&
                           {
                               return _s.grid.major_every;
                           }));

    return;
}

// cells_part
//   the grid's cells and columns.
void
cells_part(
    std::vector<sheet_setting>& _out
)
{
    _out.push_back(flag("grid.square_cells", "Proportional cells",
                        [](sheet& _s) -> bool&
                        {
                            return _s.grid.square_cells;
                        },
                        "every row as tall as a bit's cell is wide, times "
                        "the share below -- 100% for square cells: the "
                        "rows follow the cells' width, and the cell height "
                        "is not used"));
    _out.push_back(real("grid.cell_ratio", "Row height, of cell width", 0.4,
                        1.5,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.cell_ratio;
                        },
                        "with proportional cells, how tall a row is for a "
                        "bit's cell's width: 1.0 is square, 0.8 a fifth "
                        "shorter"));
    _out.push_back(real("grid.cell_width", "Cell width", 8.0, 56.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.cell_width;
                        },
                        "a bit's cell's width; set by hand, it is the width "
                        "the cells take, and the room to spare goes to "
                        "nothing"));
    _out.push_back(real("grid.cell_height", "Cell height", 12.0, 56.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.cell_height;
                        }));
    _out.push_back(real("grid.cell_gap", "Cell gap", 0.0, 8.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.cell_gap;
                        }));
    _out.push_back(real("grid.rounding", "Rounding", 0.0, 12.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.rounding;
                        }));
    _out.push_back(real("grid.name_width", "Name column", 80.0, 1000.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.name_width;
                        }));
    _out.push_back(real("grid.value_width", "Value column", 0.0, 600.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.grid.value_width;
                        }));
    _out.push_back(real("grid.indent", "Indent", 0.0, 40.0,
                        [](sheet& _s) -> float& { return _s.grid.indent; }));

    return;
}

// segments_setting
//   the automatic segments: a choice whose values are 0, 4, 8 and 16 bits.
sheet_setting
segments_setting()
{
    sheet_setting made;

    made.path    = "grid.auto_segments";
    made.label   = "Automatic segments";
    made.kind    = setting_kind::choice;
    made.choices = { "none", "nibbles", "bytes", "half-words" };
    made.words   = { "none", "nibbles", "bytes", "half_words" };
    made.high    = 3.0;
    made.read    = [](const sheet& _s)
    {
        return (_s.grid.auto_segments == 4)  ? 1.0
             : (_s.grid.auto_segments == 8)  ? 2.0
             : (_s.grid.auto_segments == 16) ? 3.0
                                             : 0.0;
    };
    made.write   = [](sheet& _s, double _value)
    {
        const long index = std::lround(_value);

        _s.grid.auto_segments = (index == 1) ? 4
                              : (index == 2) ? 8
                              : (index == 3) ? 16
                                             : 0;
    };

    return made;
}

// names_by_setting
//   what the symbols column writes for each row -- the buttons over it,
// and their menu: its symbol name, its own name, or its value, written as
// the header writes it or, lined up, every row alike. Values leave the
// macro names alone, which still say how a range's name is written across
// its cells; one choice is one setting, and one step to undo.
sheet_setting
names_by_setting()
{
    sheet_setting made;

    made.path    = "grid.names_by";
    made.label   = "Symbols by";
    made.kind    = setting_kind::choice;
    made.choices = { "symbol name", "name", "value, as written",
                     "value in hex", "value in decimal",
                     "value in binary", "value as a shifted run",
                     "value as the bits it sets" };
    made.words   = { "symbol", "name", "value", "hex", "dec", "bin",
                     "shift", "bits" };
    made.high    = static_cast<double>(NAMES_BY_VALUE +
                                       VALUE_FORM_COUNT - 1);
    made.help    = "what the symbols column writes for each row: its "
                   "symbol name, its own name, or its value -- as the "
                   "header writes it, or every row alike, lined up";
    made.read    = [](const sheet& _s)
    {
        return (_s.grid.names_by_value)
                   ? static_cast<double>(NAMES_BY_VALUE +
                                         static_cast<int>(
                                             _s.grid.names_value_form))
             : (_s.grid.full_names) ? 0.0
                                    : 1.0;
    };
    made.write   = [](sheet& _s, double _value)
    {
        const long index = std::clamp<long>(
            std::lround(_value),
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

    return made;
}

// grid_flag / colour_role / gen_flag / gen_text
//   structs: a setting by the member it reads and writes -- the grid's flag,
// a colour's role, the generator's flag or text -- with its path, its label
// and, for a flag, its help.
struct grid_flag
{
    const char*          path;
    const char*          label;
    bool grid_settings::*member;
    const char*          help;
};

struct colour_role
{
    const char*            path;
    const char*            label;
    rgba colour_settings::*member;
};

struct gen_flag
{
    const char*         path;
    const char*         label;
    bool gen_settings::*member;
};

struct gen_text
{
    const char*                path;
    const char*                label;
    std::string gen_settings::*member;
};

// GRID_FLAGS
//   constant: the grid's flags, as its Columns panel lists them.
const grid_flag GRID_FLAGS[] = {
    { "grid.show_indices", "Bit indices", &grid_settings::show_indices, "" },
    { "grid.show_digits", "Digits in cells", &grid_settings::show_digits,
      "" },
    { "grid.show_names", "Symbols column", &grid_settings::show_names, "" },
    { "grid.show_values", "Value column", &grid_settings::show_values, "" },
    { "grid.show_bits", "Bit/byte visualizer", &grid_settings::show_bits,
      "" },
    { "grid.values_first", "Value before Symbols",
      &grid_settings::values_first, "" },
    { "grid.show_labels", "Names across ranges", &grid_settings::show_labels,
      "" },
    { "grid.show_colours", "Colours in cells", &grid_settings::show_colours,
      "the rows' colours in their cells: off, their bits are drawn plain -- "
      "the grid's display rules may show them in places" },
    { "grid.show_cell_boxes", "Cell boxes", &grid_settings::show_cell_boxes,
      "draw each cell as its own rounded box" },
    { "grid.show_probe", "Probe row", &grid_settings::show_probe, "" },
    { "grid.show_usage", "Usage row", &grid_settings::show_usage, "" },
    { "grid.full_names", "Macro names in the grid",
      &grid_settings::full_names, "" } };

// COLOUR_ROLES
//   constant: the colours a role takes, the theme's while they are 0.
const colour_role COLOUR_ROLES[] = {
    { "colours.accent", "Accent", &colour_settings::accent },
    { "colours.empty_cell", "Empty cell", &colour_settings::empty_cell },
    { "colours.collision", "Collision", &colour_settings::collision },
    { "colours.probe", "Probe", &colour_settings::probe },
    { "colours.selection", "Selection", &colour_settings::selection },
    { "colours.hover", "Hover", &colour_settings::hover },
    { "colours.ghost", "Preview outline", &colour_settings::ghost } };

// GEN_FLAGS / GEN_TEXTS
//   constants: the generator's flags and texts, as its panel lists them.
const gen_flag GEN_FLAGS[] = {
    { "gen.upper_hex", "Upper-case hex", &gen_settings::upper_hex },
    { "gen.upper_suffix", "Upper-case suffix", &gen_settings::upper_suffix },
    { "gen.pad", "Pad to the width", &gen_settings::pad },
    { "gen.banner", "Banner", &gen_settings::banner },
    { "gen.guard", "Include guard", &gen_settings::guard },
    { "gen.pragma_once", "#pragma once instead", &gen_settings::pragma_once },
    { "gen.comments", "Briefs", &gen_settings::comments },
    { "gen.sections", "Numbered sections", &gen_settings::sections },
    { "gen.align", "Align values", &gen_settings::align } };

const gen_text GEN_TEXTS[] = {
    { "gen.file_name", "File name", &gen_settings::file_name },
    { "gen.guard_name", "Guard (empty: derived)", &gen_settings::guard_name },
    { "gen.name_space", "C++ namespace", &gen_settings::name_space },
    { "gen.author", "Author", &gen_settings::author } };

// columns_part
//   the grid's columns, labels and order.
void
columns_part(
    std::vector<sheet_setting>& _out
)
{
    _out.push_back(segments_setting());
    _out.push_back(integer("grid.index_every", "Label every n bits", 1, 16,
                           [](sheet& _s) -> int&
                           {
                               return _s.grid.index_every;
                           }));

    for (const grid_flag& each : GRID_FLAGS)
    {
        bool grid_settings::* const member = each.member;

        _out.push_back(flag(each.path, each.label,
                            [member](sheet& _s) -> bool&
                            {
                                return _s.grid.*member;
                            },
                            each.help));
    }

    _out.push_back(names_by_setting());

    return;
}

// order_part
//   the order the rows are shown in, and what is drawn of what is unused:
// the sheet's own order, or each group's sorted -- the grid and the tree
// alike, the header never.
void
order_part(
    std::vector<sheet_setting>& _out
)
{
    sheet_setting sort_by = choice<sort_key>(
        "grid.sort_by", "Sort the rows by",
        { "the sheet's own order", "symbol name", "name", "short name",
          "value", "lowest bit", "width", "kind", "note" },
        { "sheet", "symbol", "name", "short_name", "value", "lowest",
          "width", "kind", "note" },
        [](sheet& _s) -> sort_key& { return _s.grid.sort_by; });

    sort_by.help = "the order the grid and the tree show each group's rows "
                   "in: the sheet's own, or sorted -- groups first, a row "
                   "the key says nothing of last. The header keeps the "
                   "sheet's order";
    _out.push_back(std::move(sort_by));
    _out.push_back(flag("grid.sort_descending", "Sort descending",
                        [](sheet& _s) -> bool&
                        {
                            return _s.grid.sort_descending;
                        },
                        "sorted from the last to the first: Z to A, the "
                        "largest value first"));
    _out.push_back(flag("grid.abbreviate_unused", "Abbreviate unused runs",
                        [](sheet& _s) -> bool&
                        {
                            return _s.grid.abbreviate_unused;
                        },
                        "runs of bits no visible row uses shrink to one "
                        "narrow column"));
    _out.push_back(integer("grid.unused_run", "Shortest run abbreviated", 2,
                           32,
                           [](sheet& _s) -> int&
                           {
                               return _s.grid.unused_run;
                           }));
    _out.push_back(choice<collapsed_style>(
        "grid.collapsed", "A collapsed segment shows",
        { "its bits in hex", "a dot where set", "how many are set",
          "nothing" },
        { "hex", "dots", "count", "blank" },
        [](sheet& _s) -> collapsed_style& { return _s.grid.collapsed; }));

    sheet_setting fill = choice<grid_fill>(
        "grid.fill", "Room to spare goes to",
        { "the bits' cells", "the name column", "nothing" },
        { "cells", "names", "none" },
        [](sheet& _s) -> grid_fill& { return _s.grid.fill; });

    fill.help = "when the grid is wider than its columns need: the cells "
                "widen alike, or the name column takes it all, or the grid "
                "ends where its columns do";
    _out.push_back(std::move(fill));

    return;
}

// colours_part
//   the sheet's colours: the theme is the window's (the Theme menu); the
// palette, the scheme's unless the sheet chooses one.
void
colours_part(
    std::vector<sheet_setting>& _out
)
{
    std::vector<std::string> palettes = { "the scheme's" };
    words                    named    = { "scheme" };

    for (int i = 0; i < SCHEME_COUNT; ++i)
    {
        palettes.emplace_back(scheme_name(i));
        named.push_back(word_of(scheme_name(i)));
    }

    sheet_setting palette = choice<int>(
        "colours.palette", "Palette", palettes, named,
        [](sheet& _s) -> int& { return _s.colours.palette; },
        -1);

    palette.help = "the colours rows are given when they have none of "
                   "their own; the scheme's follows the Theme menu";
    _out.push_back(std::move(palette));
    _out.push_back(theme_real("colours.saturation", "Saturation", 0.0, 1.0,
                              [](sheet& _s) -> float&
                              {
                                  return _s.colours.saturation;
                              }));
    _out.push_back(theme_real("colours.value", "Brightness", 0.1, 1.0,
                              [](sheet& _s) -> float&
                              {
                                  return _s.colours.value;
                              }));
    _out.push_back(choice<colour_mode>(
        "colours.mode", "Colour set bits by",
        { "their row", "their group", "their kind", "the accent" },
        { "row", "group", "kind", "accent" },
        [](sheet& _s) -> colour_mode& { return _s.colours.mode; }));
    _out.push_back(real("colours.set_alpha", "Set-bit opacity", 0.1, 1.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.colours.set_alpha;
                        }));
    _out.push_back(real("colours.range_alpha", "Range tint", 0.0, 1.0,
                        [](sheet& _s) -> float&
                        {
                            return _s.colours.range_alpha;
                        }));
    for (const colour_role& each : COLOUR_ROLES)
    {
        rgba colour_settings::* const member = each.member;

        _out.push_back(colour(each.path, each.label,
                              [member](sheet& _s) -> rgba&
                              {
                                  return _s.colours.*member;
                              }));
    }

    return;
}

// level_setting
//   the language level, which decides the language as well.
sheet_setting
level_setting()
{
    std::vector<std::string> levels;
    words                    named;

    for (int i = 0; i < STANDARD_COUNT; ++i)
    {
        const char* const name = standard_name(static_cast<standard>(i));
        std::string       word;

        // "C++17" is cpp17, "C99" c99, as the sheet file writes them
        for (const char* at = name; *at != '\0'; ++at)
        {
            word += (*at == '+')
                    ? 'p'
                    : static_cast<char>(std::tolower(
                          static_cast<unsigned char>(*at)));
        }

        levels.emplace_back(name);
        named.push_back(word);
    }

    sheet_setting made = choice<standard>(
        "gen.level", "Language level", levels, named,
        [](sheet& _s) -> standard& { return _s.gen.level; });

    made.write = [](sheet& _s, double _value)
    {
        const long index = std::clamp<long>(std::lround(_value),
                                            0,
                                            STANDARD_COUNT - 1);

        // the level decides the language
        _s.gen.level = static_cast<standard>(index);
        _s.gen.lang  = is_cpp(_s.gen.level) ? language::cpp : language::c;
    };

    return made;
}

// word_setting
//   a wide sheet's arrays: words of 8, 16, 32 or 64 bits.
sheet_setting
word_setting()
{
    sheet_setting made;

    made.path    = "gen.word";
    made.label   = "Array words";
    made.kind    = setting_kind::choice;
    made.choices = { "8 bits", "16 bits", "32 bits", "64 bits" };
    made.words   = { "8", "16", "32", "64" };
    made.high    = 3.0;
    made.help    = "a sheet wider than an integer is written as an array "
                   "of words this wide";
    made.read    = [](const sheet& _s)
    {
        switch (word_bits(_s))
        {
            case 8:  return 0.0;
            case 16: return 1.0;
            case 64: return 3.0;
            default: return 2.0;
        }
    };
    made.write   = [](sheet& _s, double _value)
    {
        const long index = std::clamp<long>(std::lround(_value),
                                            0,
                                            3);

        _s.gen.word = 8 << index;
    };

    return made;
}

// generator_part
//   how the header is written.
void
generator_part(
    std::vector<sheet_setting>& _out
)
{
    _out.push_back(level_setting());
    _out.push_back(choice<gen_form>(
        "gen.form", "Form", { "macros", "C++ constants" },
        { "macros", "constants" },
        [](sheet& _s) -> gen_form& { return _s.gen.form; }));
    _out.push_back(choice<gen_style>(
        "gen.style", "Style", { "djinterp", "plain" },
        { "djinterp", "plain" },
        [](sheet& _s) -> gen_style& { return _s.gen.style; }));
    _out.push_back(choice<radix>(
        "gen.base", "Literals in", { "hex", "decimal", "octal", "binary",
                                     "shifts" },
        { "hex", "dec", "oct", "bin", "shift" },
        [](sheet& _s) -> radix& { return _s.gen.base; }));
    _out.push_back(choice<int_suffix>(
        "gen.suffix", "Integer suffix", { "none", "u", "ul", "ull" },
        { "none", "u", "ul", "ull" },
        [](sheet& _s) -> int_suffix& { return _s.gen.suffix; }));
    for (const gen_flag& each : GEN_FLAGS)
    {
        bool gen_settings::* const member = each.member;

        _out.push_back(flag(each.path, each.label,
                            [member](sheet& _s) -> bool&
                            {
                                return _s.gen.*member;
                            }));
    }

    for (const gen_text& each : GEN_TEXTS)
    {
        std::string gen_settings::* const member = each.member;

        _out.push_back(text(each.path, each.label,
                            [member](sheet& _s) -> std::string&
                            {
                                return _s.gen.*member;
                            }));
    }

    _out.push_back(word_setting());

    sheet_setting model = choice<data_model>(
        "gen.model", "Data model",
        { "LP64 (Linux, macOS)", "LLP64 (Windows)", "ILP32 (32-bit)" },
        { "lp64", "llp64", "ilp32" },
        [](sheet& _s) -> data_model& { return _s.gen.model; });

    model.help = "the widths C leaves to the target -- long's, a "
                 "pointer's -- as macros are evaluated";
    _out.push_back(std::move(model));

    return;
}

// make_settings
//   the table, in the order the window's panels list it.
std::vector<sheet_setting>
make_settings()
{
    std::vector<sheet_setting> out;

    sheet_part(out);
    lines_part(out);
    cells_part(out);
    columns_part(out);
    order_part(out);
    colours_part(out);
    generator_part(out);

    return out;
}

// AUTOMATIC
//   constant: the word for the theme's and scheme's choice.
const char* const AUTOMATIC = "auto";

// lowered
//   ASCII text in lower case.
std::string
lowered(
    const std::string& _text
)
{
    std::string out = _text;

    for (char& each : out)
    {
        each = static_cast<char>(std::tolower(static_cast<unsigned char>(
                   each)));
    }

    return out;
}

/*
separated_digits
  The characters from `_at` with each `_` taken out, which must stand
between two others; false for one that does not.
*/
bool
separated_digits(
    const std::string& _text,
    std::size_t        _at,
    std::string&       _digits
)
{
    bool last_digit = false;

    for (std::size_t at = _at; at < _text.size(); ++at)
    {
        const char each  = _text[at];
        const bool apart = ( (each == '_')              &&
                             (last_digit)               &&
                             (at + 1u < _text.size())   &&
                             (_text[at + 1u] != '_') );

        if ( (each == '_') &&
             (!apart) )
        {
            return false;
        }

        _digits    += (each == '_') ? std::string() : std::string(1u, each);
        last_digit  = (each != '_');
    }

    return true;
}

/*
read_whole
  An integer as the command line writes one: a sign, then decimal digits,
or 0x, 0o or 0b and the base's digits, `_` between digits; a leading zero
is not octal. False for anything else, or past what a long holds.
*/
bool
read_whole(
    const std::string& _text,
    long&              _value
)
{
    std::size_t at       = 0;
    const bool  negative = ( (!_text.empty()) &&
                             (_text[0] == '-') );

    at += ( (!_text.empty()) &&
            ( (_text[0] == '-') ||
              (_text[0] == '+') ) ) ? 1u : 0u;

    int base = 10;

    // a base's prefix
    if ( (_text.size() > at + 1u) &&
         (_text[at] == '0') )
    {
        const char mark = static_cast<char>(std::tolower(
                              static_cast<unsigned char>(_text[at + 1u])));

        base = (mark == 'x') ? 16 : (mark == 'o') ? 8 : (mark == 'b') ? 2
                                                                      : 10;
        at  += (base != 10) ? 2u : 0u;
    }

    std::string digits;

    if (!separated_digits(_text, at, digits))
    {
        return false;
    }

    long        magnitude = 0;
    const char* first     = digits.data();
    const char* end       = digits.data() + digits.size();
    const auto  result    = std::from_chars(first,
                                            end,
                                            magnitude,
                                            base);

    // every digit read, none of another base
    if ( (digits.empty())               ||
         (result.ec != std::errc())     ||
         (result.ptr != end) )
    {
        return false;
    }

    _value = (negative) ? -magnitude : magnitude;

    return true;
}

/*
read_real
  A real in the C locale's form, whole; locale-independent where the library
has from_chars for doubles.
*/
bool
read_real(
    const std::string& _text,
    double&            _value
)
{
    // nothing, or a sign alone
    if ( (_text.empty()) ||
         (_text == "+") ||
         (_text == "-") )
    {
        return false;
    }

#if defined(__cpp_lib_to_chars)
    const char* const first = _text.data() +
                              ((_text[0] == '+') ? 1 : 0);
    const char* const end   = _text.data() + _text.size();
    const auto        found = std::from_chars(first,
                                              end,
                                              _value);

    return ( (found.ec == std::errc()) &&
             (found.ptr == end)        &&
             (std::isfinite(_value)) );
#else
    char*        end   = nullptr;
    const double value = std::strtod(_text.c_str(),
                                     &end);

    if ( (end == nullptr) ||
         (*end != '\0')   ||
         (!std::isfinite(value)) )
    {
        return false;
    }

    _value = value;

    return true;
#endif
}

// real_text
//   a float's value as the shortest text that reads back as it.
std::string
real_text(
    double _value
)
{
    char        buffer[64];
    const float held = static_cast<float>(_value);

#if defined(__cpp_lib_to_chars)
    const auto written = std::to_chars(buffer,
                                       buffer + sizeof(buffer),
                                       held);

    return std::string(buffer,
                       written.ptr);
#else
    (void)std::snprintf(buffer,
                        sizeof(buffer),
                        "%g",
                        static_cast<double>(held));

    return std::string(buffer);
#endif
}

// read_flag / read_integer / read_colour / read_choice
//   a value as each kind writes it, read into what `write` takes: true,
// yes, on or 1, or their opposites, in any case; an integer within the
// bounds; #RRGGBB or #RRGGBBAA as its packed bits; a choice's word as its
// index. False for anything else.
bool
read_flag(
    const std::string& _text,
    double&            _value
)
{
    static const char* const YES[] = { "true", "yes", "on", "1" };
    static const char* const NO[]  = { "false", "no", "off", "0" };
    const std::string        said  = lowered(_text);

    for (std::size_t i = 0; i < 4u; ++i)
    {
        if ( (said == YES[i]) ||
             (said == NO[i]) )
        {
            _value = (said == YES[i]) ? 1.0 : 0.0;

            return true;
        }
    }

    return false;
}

bool
read_integer(
    const sheet_setting& _setting,
    const std::string&   _text,
    double&              _value
)
{
    long value = 0;

    _value = 0.0;

    if (!read_whole(_text, value))
    {
        return false;
    }

    _value = static_cast<double>(value);

    return ( (_value >= _setting.low) &&
             (_value <= _setting.high) );
}

bool
read_colour(
    const std::string& _text,
    double&            _value
)
{
    rgba value = 0;

    if ( (_text.empty())     ||
         (_text[0] != '#')   ||
         (!parse_rgba(_text, value)) )
    {
        return false;
    }

    _value = static_cast<double>(value);

    return true;
}

bool
read_choice(
    const sheet_setting& _setting,
    const std::string&   _text,
    double&              _value
)
{
    for (std::size_t i = 0; i < _setting.words.size(); ++i)
    {
        if (_setting.words[i] == _text)
        {
            _value = static_cast<double>(i);

            return true;
        }
    }

    return false;
}

// bound_text
//   a bound as help writes it: an integer's as one, a real's shortest.
std::string
bound_text(
    const sheet_setting& _setting,
    double               _bound
)
{
    return (_setting.kind == setting_kind::integer)
           ? std::to_string(std::lround(_bound))
           : real_text(_bound);
}

}  // anonymous namespace


/*
sheet_settings / find_sheet_setting
  The table, made once; a path found by walking it, which at a hundred
entries is quicker than anything that must be built first.
*/
const std::vector<sheet_setting>&
sheet_settings()
{
    static const std::vector<sheet_setting> table = make_settings();

    return table;
}

const sheet_setting*
find_sheet_setting(
    const std::string& _path
)
{
    for (const sheet_setting& each : sheet_settings())
    {
        if (each.path == _path)
        {
            return &each;
        }
    }

    return nullptr;
}

/*
is_preference
  Everything but the sheet's content, which is listed.
*/
bool
is_preference(
    const std::string& _path
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
        if (_path == each)
        {
            return false;
        }
    }

    return true;
}

/*
setting_text
  The value read, written as its kind writes it.
*/
std::string
setting_text(
    const sheet_setting& _setting,
    const sheet&         _sheet
)
{
    switch (_setting.kind)
    {
        case setting_kind::flag:
            return (_setting.read(_sheet) != 0.0) ? "true" : "false";

        case setting_kind::integer:
            return std::to_string(std::lround(_setting.read(_sheet)));

        case setting_kind::real:
            return ( (_setting.automatic) &&
                     (_setting.read(_sheet) < _setting.low) )
                   ? std::string(AUTOMATIC)
                   : real_text(_setting.read(_sheet));

        case setting_kind::colour:
            return ( (_setting.automatic) &&
                     (_setting.read(_sheet) == 0.0) )
                   ? std::string(AUTOMATIC)
                   : rgba_text(static_cast<rgba>(_setting.read(_sheet)));

        case setting_kind::choice:
        {
            const long index = std::lround(_setting.read(_sheet));

            return ( (index >= 0) &&
                     (static_cast<std::size_t>(index) <
                      _setting.words.size()) )
                   ? _setting.words[static_cast<std::size_t>(index)]
                   : std::string();
        }

        case setting_kind::text:
            break;
    }

    return _setting.read_text(_sheet);
}

/*
set_setting
  `auto` where the theme may choose; otherwise the text read as its kind
reads it, checked against the bounds -- the write would hold a value to
them, which a script should hear of -- and only then written.
*/
bool
set_setting(
    sheet&               _sheet,
    const sheet_setting& _setting,
    const std::string&   _value,
    std::string&         _error)
{
    double value = 0.0;
    bool   read  = false;

    // the theme's and scheme's again: a colour's 0, a real's -1
    if ( (_setting.automatic) &&
         (lowered(_value) == AUTOMATIC) )
    {
        _setting.write(_sheet,
                       (_setting.kind == setting_kind::colour) ? 0.0 : -1.0);

        return true;
    }

    switch (_setting.kind)
    {
        case setting_kind::flag:
            read = read_flag(_value, value);
            break;

        case setting_kind::integer:
            read = read_integer(_setting, _value, value);
            break;

        case setting_kind::real:
            read = ( read_real(_value, value) &&
                     (value >= _setting.low)  &&
                     (value <= _setting.high) );
            break;

        case setting_kind::colour:
            read = read_colour(_value, value);
            break;

        case setting_kind::choice:
            read = read_choice(_setting, _value, value);
            break;

        case setting_kind::text:
            _setting.write_text(_sheet,
                                _value);

            return true;
    }

    if (read)
    {
        _setting.write(_sheet,
                       value);

        return true;
    }

    _error = "'" + _value + "' is not " + setting_values(_setting);

    return false;
}

/*
setting_values
  The kind's words, and a choice's own, joined as English joins a list.
*/
std::string
setting_values(
    const sheet_setting& _setting
)
{
    switch (_setting.kind)
    {
        case setting_kind::flag:
            return "true or false";

        case setting_kind::integer:
            return "an integer from " + bound_text(_setting, _setting.low) +
                   " to " + bound_text(_setting, _setting.high);

        case setting_kind::real:
            return "a number from " + bound_text(_setting, _setting.low) +
                   " to " + bound_text(_setting, _setting.high) +
                   (_setting.automatic ? ", or auto" : "");

        case setting_kind::colour:
            return std::string("a colour, #RRGGBB or #RRGGBBAA") +
                   (_setting.automatic ? ", or auto" : "");

        case setting_kind::choice:
        {
            std::string out;

            for (std::size_t i = 0; i < _setting.words.size(); ++i)
            {
                out += (i == 0u)                            ? ""
                     : (i + 1u == _setting.words.size())   ? " or "
                                                            : ", ";
                out += _setting.words[i];
            }

            return out;
        }

        case setting_kind::text:
            break;
    }

    return "text";
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
