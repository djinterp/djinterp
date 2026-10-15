/*******************************************************************************
* bitwisedom [app]                                                  settings.cpp
*
* The settings registry.
*   Each entry is made by a small factory per kind, from a function that
* reaches the field in a sheet. One reach serves both directions: reading
* takes the field, writing assigns it, so the two cannot disagree on which
* field a path names.
*
* path:      /src/djinterp/tools/bitwisedom/app/settings.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./settings.hpp"  // corresponding header

// std
#include <algorithm>  // std::clamp
#include <cmath>      // std::lround
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/codegen.hpp>  // standard_name


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
    Reach       _reach
)
{
    setting made;
    made.path  = _path;
    made.label = _label;
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
    out.push_back(real("grid.cell_width", "Cell width", 8.0, 56.0,
                       [](sheet& _s) -> float& { return _s.grid.cell_width; }));
    out.push_back(real("grid.cell_height", "Cell height", 12.0, 56.0,
                       [](sheet& _s) -> float&
                       {
                           return _s.grid.cell_height;
                       }));
    out.push_back(real("grid.cell_gap", "Cell gap", 0.0, 8.0,
                       [](sheet& _s) -> float& { return _s.grid.cell_gap; }));
    out.push_back(real("grid.rounding", "Rounding", 0.0, 12.0,
                       [](sheet& _s) -> float& { return _s.grid.rounding; }));
    out.push_back(real("grid.name_width", "Name column", 80.0, 520.0,
                       [](sheet& _s) -> float& { return _s.grid.name_width; }));
    out.push_back(real("grid.value_width", "Value column", 0.0, 320.0,
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
    out.push_back(flag("grid.show_labels", "Names across ranges",
                       [](sheet& _s) -> bool&
                       {
                           return _s.grid.show_labels;
                       }));
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

}  // namespace gui
}  // namespace bitwisedom
