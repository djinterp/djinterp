/*******************************************************************************
* bitwisedom [bitwisedom]                                              sheet.cpp
*
* The sheet: colours, defaults, and the operations on its tree of rows.
*   The tree is a value -- rows own their children by value -- so lookups
* walk it rather than index it. A sheet holds tens or hundreds of rows, and
* every edit copies it whole for undo anyway; an index would be one more
* thing to keep in step.
*
* path:      /src/djinterp/tools/bitwisedom/sheet.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/sheet.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::clamp, std::max
#include <cmath>      // std::floor, std::fmod
#include <cstddef>    // std::size_t
#include <utility>    // std::move


namespace bitwisedom
{

namespace
{

// GOLDEN
//   constant: the golden ratio's fractional part, which spreads successive
// hues as far apart as they can be.
const float GOLDEN = 0.61803398875f;

// to_byte
//   a channel in [0, 1] as a byte.
re_std::uint32_t
to_byte(
    float _channel
)
{
    const float clamped = std::clamp(_channel, 0.0f, 1.0f);

    return static_cast<re_std::uint32_t>(clamped * 255.0f + 0.5f);
}

// hex_value
//   one hexadecimal digit's value, or -1.
int
hex_value(
    char _digit
)
{
    if ( (_digit >= '0') &&
         (_digit <= '9') )
    {
        return _digit - '0';
    }

    if ( (_digit >= 'a') &&
         (_digit <= 'f') )
    {
        return _digit - 'a' + 10;
    }

    if ( (_digit >= 'A') &&
         (_digit <= 'F') )
    {
        return _digit - 'A' + 10;
    }

    return -1;
}

// assign_ids
//   gives a row and all of its descendants fresh ids.
void
assign_ids(
    sheet& _sheet,
    row&   _row
)
{
    _row.id = _sheet.next_id++;

    for (row& child : _row.children)
    {
        assign_ids(_sheet, child);
    }

    return;
}

// find_in
//   the row with an id below (or at) a row, and its parent.
row*
find_in(
    row&  _at,
    int   _id,
    row** _parent
)
{
    if (_at.id == _id)
    {
        return &_at;
    }

    for (row& child : _at.children)
    {
        if (child.id == _id)
        {
            if (_parent != nullptr)
            {
                *_parent = &_at;
            }

            return &child;
        }

        row* found = find_in(child, _id, _parent);

        if (found != nullptr)
        {
            return found;
        }
    }

    return nullptr;
}

// walk_from
//   the depth-first walk below one row.
void
walk_from(
    const row&                                              _parent,
    int                                                     _depth,
    const std::function<void(const row&, const row&, int)>& _visit
)
{
    for (const row& child : _parent.children)
    {
        _visit(child, _parent, _depth);
        walk_from(child, _depth + 1, _visit);
    }

    return;
}

// contains
//   whether a row's subtree holds an id.
bool
contains(
    const row& _row,
    int        _id
)
{
    if (_row.id == _id)
    {
        return true;
    }

    for (const row& child : _row.children)
    {
        if (contains(child, _id))
        {
            return true;
        }
    }

    return false;
}

// open_within
//   _row, unless it is the root, and every group within it, opened or
// closed; how many of them changed.
int
open_within(
    row& _row,
    bool _open,
    bool _root
)
{
    int changed = 0;

    // a group not yet as asked; the root is shown open, whatever it says
    if ( (!_root) &&
         (can_contain(_row)) &&
         ((_row.kind == row_kind::group) || !_row.children.empty()) &&
         (_row.collapsed == _open) )
    {
        _row.collapsed = !_open;
        ++changed;
    }

    for (row& child : _row.children)
    {
        if (can_contain(child))
        {
            changed += open_within(child, _open, false);
        }
    }

    return changed;
}

// line
//   a line_spec, for the defaults below.
line_spec
line(
    bool       _show,
    float      _thickness,
    rgba       _colour,
    line_style _style
)
{
    line_spec spec;
    spec.show      = _show;
    spec.thickness = _thickness;
    spec.colour    = _colour;
    spec.style     = _style;

    return spec;
}

// make_template
//   one macro template, for the defaults below.
macro_template
make_template(
    bool               _enabled,
    int                _applies,
    const std::string& _name,
    const std::string& _params,
    const std::string& _body,
    const std::string& _note
)
{
    macro_template made;
    made.enabled = _enabled;
    made.applies = _applies;
    made.name    = _name;
    made.params  = _params;
    made.body    = _body;
    made.note    = _note;

    return made;
}

}  // anonymous namespace


// ------------------------------------------------------------------ colours

rgba
make_rgba(
    float _r,
    float _g,
    float _b,
    float _a
)
{
    return (to_byte(_r) << 24) |
           (to_byte(_g) << 16) |
           (to_byte(_b) << 8)  |
           to_byte(_a);
}

float
channel(
    rgba _colour,
    int  _index
)
{
    const int shift = 24 - 8 * std::clamp(_index, 0, 3);

    return static_cast<float>((_colour >> shift) & 0xFFu) / 255.0f;
}

rgba
with_alpha(
    rgba  _colour,
    float _alpha
)
{
    return (_colour & 0xFFFFFF00u) | to_byte(_alpha);
}

rgba
from_hsv(
    float _h,
    float _s,
    float _v,
    float _a
)
{
    const float h      = _h - std::floor(_h);
    const float sector = h * 6.0f;
    const int   index  = static_cast<int>(sector) % 6;
    const float f      = sector - std::floor(sector);
    const float p      = _v * (1.0f - _s);
    const float q      = _v * (1.0f - _s * f);
    const float t      = _v * (1.0f - _s * (1.0f - f));

    switch (index)
    {
        case 0:  return make_rgba(_v, t, p, _a);
        case 1:  return make_rgba(q, _v, p, _a);
        case 2:  return make_rgba(p, _v, t, _a);
        case 3:  return make_rgba(p, q, _v, _a);
        case 4:  return make_rgba(t, p, _v, _a);
        default: return make_rgba(_v, p, q, _a);
    }
}

/*
palette_colour
  Successive indices step round the hue circle by the golden ratio, so any
run of neighbours is well spread. Each palette then bends the hue, saturation
and value: pastel washes the colour out, earth folds the hues into the warm
half, neon pins the saturation to full. The colour-blind-safe palette is
Okabe and Ito's seven, which stay distinct to the common colour-vision
deficiencies; past the seventh it goes round again darker, then lighter.
*/
rgba
palette_colour(
    int   _palette,
    int   _index,
    float _saturation,
    float _value
)
{
    const float hue = std::fmod(0.08f + static_cast<float>(_index) * GOLDEN,
                                1.0f);

    switch (_palette)
    {
        case 1:  // pastel
            return from_hsv(hue, _saturation * 0.45f, _value, 1.0f);

        case 2:  // earth
            return from_hsv(0.02f + hue * 0.36f,
                            _saturation * 0.75f,
                            _value * 0.78f,
                            1.0f);

        case 3:  // neon
            return from_hsv(hue, 1.0f, _value, 1.0f);

        case 4:  // colour-blind safe
        {
            static const rgba okabe_ito[7] = {
                0xE69F00FFu, 0x56B4E9FFu, 0x009E73FFu, 0xF0E442FFu,
                0x0072B2FFu, 0xD55E00FFu, 0xCC79A7FFu };

            const int  index = (_index < 0) ? -_index : _index;
            const rgba base  = okabe_ito[index % 7];
            const int  round = (index / 7) % 3;

            // round 0 as it is, 1 toward black, 2 toward white; and the
            // whole held to the scheme's brightness
            float channels[3];

            for (int i = 0; i < 3; ++i)
            {
                const float c = channel(base, i);

                channels[i] = ( (round == 1) ? c * 0.70f
                              : (round == 2) ? c + (1.0f - c) * 0.35f
                                             : c ) * _value;
            }

            return make_rgba(channels[0], channels[1], channels[2], 1.0f);
        }

        default: // vivid
            return from_hsv(hue, _saturation, _value, 1.0f);
    }
}

rgba
or_else(
    rgba _colour,
    rgba _automatic
)
{
    return (_colour != 0) ? _colour
                          : _automatic;
}


// ------------------------------------------------------- themes and schemes

const char*
theme_name(
    int _theme
)
{
    return (_theme == 1) ? "Light"
                         : "Dark";
}

const char*
scheme_name(
    int _scheme
)
{
    static const char* const names[SCHEME_COUNT] = {
        "Vivid", "Pastel", "Earth", "Neon", "Colour-blind safe" };

    return names[std::clamp(_scheme, 0, SCHEME_COUNT - 1)];
}

/*
scheme_for
  Every scheme starts from its theme's neutral surfaces and roles, and
changes what makes it itself: its palette and accent always; earth warms
the surfaces and the lines, neon cools and deepens them, and the
colour-blind-safe scheme takes its collision and probe colours from its
own palette. A light theme's palettes are darker and more saturated than a
dark one's, to stand out against white.
*/
scheme_colours
scheme_for(
    int _theme,
    int _scheme
)
{
    const bool     light  = (_theme == 1);
    const int      scheme = std::clamp(_scheme, 0, SCHEME_COUNT - 1);
    scheme_colours out;

    // the neutral base: the window's surfaces, the roles, the lines
    if (light)
    {
        out.window   = 0xF2F5F7FFu;
        out.panel    = 0xFCFCFFFFu;
        out.frame    = 0xE6E8F0FFu;
        out.text     = 0x1A1C21FFu;
        out.colours.accent     = 0x2F6FE0FFu;
        out.colours.empty_cell = 0xEEF0F4FFu;
        out.colours.collision  = 0xE03131FFu;
        out.colours.probe      = 0xE8A317FFu;
        out.colours.hover      = 0x0000001Au;
        out.colours.ghost      = 0x202020C0u;
        out.colours.saturation = 0.70f;
        out.colours.value      = 0.82f;
        out.minor    = 0xC9CED8FFu;
        out.major    = 0x7A8294FFu;
        out.rows     = 0xDDE1E8FFu;
        out.groups   = 0x9AA1B0FFu;
        out.border   = 0x9AA1B0FFu;
    }
    else
    {
        out.window   = 0x1C1F24FFu;
        out.panel    = 0x171A1FFFu;
        out.frame    = 0x2B2E36FFu;
        out.text     = 0xE3E6EBFFu;
        out.colours.accent     = 0x4C8DFFFFu;
        out.colours.empty_cell = 0x2A2E36FFu;
        out.colours.collision  = 0xFF4D4DFFu;
        out.colours.probe      = 0xFFC247FFu;
        out.colours.hover      = 0xFFFFFF1Cu;
        out.colours.ghost      = 0xFFFFFFC0u;
        out.colours.saturation = 0.62f;
        out.colours.value      = 0.92f;
        out.minor    = 0x3B4252FFu;
        out.major    = 0x7B8499FFu;
        out.rows     = 0x2E333DFFu;
        out.groups   = 0x5A6275FFu;
        out.border   = 0x5A6275FFu;
    }

    switch (scheme)
    {
        case 1:  // pastel
            out.colours.accent     = light ? 0x7C5CD6FFu : 0xB39DFFFFu;
            out.colours.saturation = light ? 0.85f : 0.62f;
            out.colours.value      = light ? 0.86f : 0.96f;
            break;

        case 2:  // earth
            out.colours.accent = light ? 0xB5651DFFu : 0xE0A050FFu;

            if (light)
            {
                out.window = 0xF2EDE0FFu;
                out.panel  = 0xFAF7EDFFu;
                out.frame  = 0xE6DECCFFu;
                out.text   = 0x29241CFFu;
                out.colours.empty_cell = 0xEDE6D6FFu;
                out.minor  = 0xD9CFBCFFu;
                out.major  = 0x8C7B6BFFu;
                out.rows   = 0xE6DECFFFu;
                out.groups = 0xB3A68FFFu;
                out.border = 0xB3A68FFFu;
            }
            else
            {
                out.window = 0x221D19FFu;
                out.panel  = 0x1B1714FFu;
                out.frame  = 0x3A322BFFu;
                out.text   = 0xEDE4D8FFu;
                out.colours.empty_cell = 0x2E2823FFu;
                out.minor  = 0x4A4038FFu;
                out.major  = 0x8C7B6BFFu;
                out.rows   = 0x362F29FFu;
                out.groups = 0x6B5D50FFu;
                out.border = 0x6B5D50FFu;
            }

            break;

        case 3:  // neon
            out.colours.accent = light ? 0x3B5BDBFFu : 0x7AA2F7FFu;
            out.colours.value  = light ? 0.85f : 1.0f;

            if (light)
            {
                out.window = 0xEEF1F8FFu;
                out.panel  = 0xF8FAFFFFu;
                out.frame  = 0xDFE5F2FFu;
                out.text   = 0x151A2BFFu;
                out.colours.empty_cell = 0xE4E9F5FFu;
                out.minor  = 0xCBD3E6FFu;
                out.major  = 0x7480A0FFu;
                out.rows   = 0xDCE2F0FFu;
                out.groups = 0xA3AEC9FFu;
                out.border = 0xA3AEC9FFu;
            }
            else
            {
                out.window = 0x0F121AFFu;
                out.panel  = 0x0A0D14FFu;
                out.frame  = 0x1C212EFFu;
                out.text   = 0xCCD6F2FFu;
                out.colours.empty_cell = 0x161A24FFu;
                out.minor  = 0x262C3AFFu;
                out.major  = 0x6B7899FFu;
                out.rows   = 0x1F2430FFu;
                out.groups = 0x3D4660FFu;
                out.border = 0x3D4660FFu;
            }

            break;

        case 4:  // colour-blind safe
            out.colours.accent    = light ? 0x0072B2FFu : 0x56B4E9FFu;
            out.colours.collision = 0xD55E00FFu;
            out.colours.probe     = light ? 0xE69F00FFu : 0xF0E442FFu;
            out.colours.value     = light ? 0.92f : 1.0f;
            break;

        default: // vivid
            break;
    }

    out.colours.theme     = light ? 1 : 0;
    out.colours.palette   = scheme;
    out.colours.selection = with_alpha(out.colours.accent,
                                       light ? 0.19f : 0.25f);
    out.segments          = with_alpha(out.colours.accent,
                                       light ? 0.69f : 0.63f);

    return out;
}

look
make_look(
    const colour_settings& _colours,
    const grid_settings&   _grid,
    int                    _theme,
    int                    _scheme
)
{
    const scheme_colours base = scheme_for(_theme, _scheme);
    look                 out;

    out.colours       = _colours;
    out.colours.theme = base.colours.theme;
    out.grid          = _grid;

    colour_settings& c = out.colours;

    c.palette    = (c.palette >= 0) ? std::min(c.palette, SCHEME_COUNT - 1)
                                    : base.colours.palette;
    c.saturation = (c.saturation >= 0.0f) ? c.saturation
                                          : base.colours.saturation;
    c.value      = (c.value >= 0.0f) ? c.value
                                     : base.colours.value;
    c.accent     = or_else(c.accent, base.colours.accent);
    c.empty_cell = or_else(c.empty_cell, base.colours.empty_cell);
    c.collision  = or_else(c.collision, base.colours.collision);
    c.probe      = or_else(c.probe, base.colours.probe);
    c.hover      = or_else(c.hover, base.colours.hover);
    c.ghost      = or_else(c.ghost, base.colours.ghost);

    // a sheet's own accent tints its selection and segment edges, unless
    // it sets those too
    const bool  light = (base.colours.theme == 1);
    c.selection = or_else(c.selection,
                          with_alpha(c.accent, light ? 0.19f : 0.25f));

    grid_settings& g = out.grid;

    g.minor.colour    = or_else(g.minor.colour, base.minor);
    g.major.colour    = or_else(g.major.colour, base.major);
    g.rows.colour     = or_else(g.rows.colour, base.rows);
    g.groups.colour   = or_else(g.groups.colour, base.groups);
    g.segments.colour = or_else(g.segments.colour,
                                with_alpha(c.accent,
                                           light ? 0.69f : 0.63f));
    g.border.colour   = or_else(g.border.colour, base.border);

    return out;
}

look
make_look(
    const sheet& _sheet,
    int          _theme,
    int          _scheme
)
{
    return make_look(_sheet.colours, _sheet.grid, _theme, _scheme);
}

std::string
rgba_text(
    rgba _colour
)
{
    return "#" + format_hex(_colour, 8, true);
}

bool
parse_rgba(
    const std::string& _text,
    rgba&              _colour
)
{
    const std::string digits = ( (!_text.empty()) &&
                                 (_text[0] == '#') ) ? _text.substr(1)
                                                     : _text;

    // six digits are opaque; eight carry alpha
    if ( (digits.size() != 6) &&
         (digits.size() != 8) )
    {
        return false;
    }

    rgba value = 0;

    for (char each : digits)
    {
        const int digit = hex_value(each);

        if (digit < 0)
        {
            return false;
        }

        value = (value << 4) | static_cast<rgba>(digit);
    }

    _colour = (digits.size() == 6) ? ((value << 8) | 0xFFu)
                                   : value;

    return true;
}


// ------------------------------------------------------------- construction

int
applies_bit(
    def_kind _kind
)
{
    return 1 << static_cast<int>(_kind);
}

std::vector<macro_template>
default_templates()
{
    const int range = applies_bit(def_kind::range);
    const int bit   = applies_bit(def_kind::bit);
    const int value = applies_bit(def_kind::field_value);

    return {
        make_template(true, range, "{NAME}_SHIFT", "", "{SHIFT}",
                      "the lowest bit of {LOCAL}"),
        make_template(true, range, "{NAME}_WIDTH", "", "{WIDTH}",
                      "how many bits {LOCAL} spans"),
        make_template(true, range, "{NAME}_GET", "x",
                      "(((x) & {NAME}) >> {SHIFT})",
                      "the value of {LOCAL} in x"),
        make_template(true, range, "{NAME}_SET", "x, v",
                      "(((x) & ~{NAME}) | (((v) << {SHIFT}) & {NAME}))",
                      "x with {LOCAL} replaced by v"),
        make_template(false, bit, "{NAME}_BIT", "", "{BIT}",
                      "the bit {LOCAL} occupies"),
        make_template(false, bit, "{NAME}_IS_SET", "x",
                      "(((x) & {NAME}) != 0)",
                      "whether {LOCAL} is set in x"),
        make_template(false, value, "{NAME}_IS", "x",
                      "(((x) & {MASK}) == {NAME})",
                      "whether x holds {LOCAL}")
    };
}

/*
default_wide_templates
  What a sheet wider than an integer writes beside its symbols, where a
symbol is a place in an array of words: a range's width, where it begins,
and its value read from and written into an array; a bit's word and mask,
and a test of it; a value compared with its field. Only the ranges' are
on to begin with, as the narrow sheet's are.
*/
std::vector<macro_template>
default_wide_templates()
{
    const int range = applies_bit(def_kind::range);
    const int bit   = applies_bit(def_kind::bit);
    const int value = applies_bit(def_kind::field_value);

    std::vector<macro_template> out = {
        make_template(true, range, "{NAME}_WIDTH", "", "{WIDTH}",
                      "how many bits {LOCAL} spans"),
        make_template(true, range, "{NAME}_WORD", "", "{WORD}",
                      "the word {LOCAL} begins in"),
        make_template(true, range, "{NAME}_SHIFT", "", "{WORD_SHIFT}",
                      "where in that word {LOCAL} begins"),
        make_template(true, range, "{NAME}_MASK", "", "{WORD_MASK}",
                      "{LOCAL}'s bits in that word"),
        make_template(true, range, "{NAME}_GET", "a", "{GET}",
                      "the value of {LOCAL} in the array a"),
        make_template(true, range, "{NAME}_SET", "a, v", "{PUT}",
                      "sets {LOCAL} in the array a to v"),
        make_template(false, bit, "{NAME}_WORD", "", "{WORD}",
                      "the word {LOCAL} is in"),
        make_template(false, bit, "{NAME}_MASK", "", "{WORD_MASK}",
                      "{LOCAL}'s bit in that word"),
        make_template(false, bit, "{NAME}_IS_SET", "a",
                      "{BITSET}TEST(a, {NAME})",
                      "whether {LOCAL} is set in the array a"),
        make_template(false, value, "{NAME}_IS", "a",
                      "({FIELD}_GET(a) == {NAME})",
                      "whether the array a holds {LOCAL}")
    };

    for (macro_template& each : out)
    {
        each.wide = true;
    }

    return out;
}

int
word_bits(
    const sheet& _sheet
)
{
    const int word = _sheet.gen.word;

    return ( (word == 8) || (word == 16) || (word == 64) ) ? word
                                                         : 32;
}

/*
reset_view
  The grid's lines and the colours as a new sheet has them: every colour
automatic, so the theme and scheme it is shown with choose them.
*/
void
reset_view(
    sheet& _sheet
)
{
    grid_settings grid;

    grid.minor    = line(true, 1.0f, 0, line_style::solid);
    grid.major    = line(true, 2.0f, 0, line_style::solid);
    grid.rows     = line(true, 1.0f, 0, line_style::solid);
    grid.groups   = line(true, 1.5f, 0, line_style::solid);
    grid.segments = line(true, 2.0f, 0, line_style::dashed);
    grid.border   = line(true, 1.0f, 0, line_style::solid);

    _sheet.grid    = grid;
    _sheet.colours = colour_settings();

    return;
}

/*
automatic_colours
  Gives every colour the sheet sets back to the theme and scheme: the roles,
the palette and its saturation and brightness, and the lines' colours. The
rows' own colours, and everything that is not a colour, are left alone.
*/
void
automatic_colours(
    sheet& _sheet
)
{
    colour_settings& c = _sheet.colours;
    grid_settings&   g = _sheet.grid;

    c.palette    = -1;
    c.saturation = -1.0f;
    c.value      = -1.0f;
    c.accent     = 0;
    c.empty_cell = 0;
    c.collision  = 0;
    c.probe      = 0;
    c.selection  = 0;
    c.hover      = 0;
    c.ghost      = 0;

    g.minor.colour    = 0;
    g.major.colour    = 0;
    g.rows.colour     = 0;
    g.groups.colour   = 0;
    g.segments.colour = 0;
    g.border.colour   = 0;

    return;
}

sheet
make_sheet(
    const std::string& _title,
    int                _width
)
{
    sheet made;
    made.title     = _title;
    made.width     = std::clamp(_width, 1, MAX_WIDTH);
    made.root.id   = 0;
    made.root.kind = row_kind::group;
    made.gen.templates = default_templates();

    for (macro_template& each : default_wide_templates())
    {
        made.gen.templates.push_back(std::move(each));
    }
    made.gen.file_name = _title + ".h";

    reset_view(made);

    return made;
}


// A new document starts with one full-width, nestable range.  make_sheet
// intentionally remains empty for importers, examples and existing callers.
sheet
make_document(const std::string& _title, int _width)
{
    sheet made = make_sheet(_title, _width);
    row root;
    root.kind = row_kind::symbol;
    root.def = def_kind::range;
    root.name = "root";
    root.lsb = 0;
    root.width = made.width;
    root.name_children = false;
    (void)insert_row(made, made.root.id, -1, root);
    return made;
}


// ------------------------------------------------------------------- lookup

row*
find_row(
    sheet& _sheet,
    int    _id
)
{
    return find_in(_sheet.root, _id, nullptr);
}

const row*
find_row(
    const sheet& _sheet,
    int          _id
)
{
    return find_in(const_cast<row&>(_sheet.root), _id, nullptr);
}

row_index::row_index(
    const sheet& _sheet
)
    : m_rows(static_cast<std::size_t>(std::max(_sheet.next_id, 1)), nullptr)
{
    // put
    //   one row, in the place its id gives it.
    auto put = [this](const row& _row)
    {
        // a negative id is no row's to be found by
        if (_row.id < 0)
        {
            return;
        }

        const std::size_t at = static_cast<std::size_t>(_row.id);

        // an id at or past next_id is the sheet's mistake, and is indexed
        // all the same
        if (at >= m_rows.size())
        {
            m_rows.resize(at + 1, nullptr);
        }

        m_rows[at] = &_row;

        return;
    };

    put(_sheet.root);
    walk(_sheet,
         [&put](const row& _row, const row&, int)
         {
             put(_row);

             return;
         });
}

const row*
row_index::find(
    int _id
) const
{
    if ( (_id < 0) ||
         (static_cast<std::size_t>(_id) >= m_rows.size()) )
    {
        return nullptr;
    }

    return m_rows[static_cast<std::size_t>(_id)];
}

row*
parent_of(
    sheet& _sheet,
    int    _id
)
{
    row* parent = nullptr;

    // the root has no parent
    if (_id == _sheet.root.id)
    {
        return nullptr;
    }

    (void)find_in(_sheet.root, _id, &parent);

    return parent;
}

void
walk(
    const sheet&                                            _sheet,
    const std::function<void(const row&, const row&, int)>& _visit
)
{
    walk_from(_sheet.root, 0, _visit);

    return;
}


// ---------------------------------------------------------------- mutation

int
insert_row(
    sheet&     _sheet,
    int        _parent,
    int        _index,
    const row& _row
)
{
    row* target = find_row(_sheet, _parent);

    if (target == nullptr)
    {
        return 0;
    }

    int index = _index;

    // a symbol cannot hold rows: the new one goes beside it
    if (!can_contain(*target))
    {
        row* parent = parent_of(_sheet, target->id);

        if (parent == nullptr)
        {
            return 0;
        }

        for (std::size_t i = 0; i < parent->children.size(); ++i)
        {
            if (parent->children[i].id == target->id)
            {
                index = static_cast<int>(i) + 1;
            }
        }

        target = parent;
    }

    row added = _row;
    assign_ids(_sheet, added);

    const int id = added.id;

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) > target->children.size()) )
    {
        target->children.push_back(std::move(added));
    }
    else
    {
        target->children.insert(target->children.begin() + index,
                                std::move(added));
    }

    return id;
}

bool
remove_row(
    sheet& _sheet,
    int    _id
)
{
    row* parent = parent_of(_sheet, _id);

    if (parent == nullptr)
    {
        return false;
    }

    for (std::size_t i = 0; i < parent->children.size(); ++i)
    {
        if (parent->children[i].id == _id)
        {
            parent->children.erase(parent->children.begin() +
                                   static_cast<std::ptrdiff_t>(i));

            return true;
        }
    }

    return false;
}

int
open_groups(
    sheet& _sheet,
    int    _id,
    bool   _open
)
{
    row* top = find_row(_sheet, _id);

    // A group or a range with children is expandable; leaf symbols are not.
    if ( (top == nullptr) ||
         (!can_contain(*top)) ||
         ((top->kind != row_kind::group) && top->children.empty()) )
    {
        return 0;
    }

    return open_within(*top, _open, (top == &_sheet.root));
}

/*
move_row
  The row is copied out, removed, and inserted again with its ids kept. A
move later within the same group counts its position before the removal, so
the index the caller sees is the one the row ends up at.
*/
bool
move_row(
    sheet& _sheet,
    int    _id,
    int    _parent,
    int    _index
)
{
    const row* moving = find_row(_sheet, _id);
    const row* target = find_row(_sheet, _parent);

    // nothing moves into itself, nor into its own subtree
    if ( (moving == nullptr)                  ||
         (target == nullptr)                  ||
         (_id == _sheet.root.id)              ||
         (!can_contain(*target))           ||
         (contains(*moving, _parent)) )
    {
        return false;
    }

    row* old_parent = parent_of(_sheet, _id);
    int  old_index  = -1;

    for (std::size_t i = 0; i < old_parent->children.size(); ++i)
    {
        if (old_parent->children[i].id == _id)
        {
            old_index = static_cast<int>(i);
        }
    }

    row copy = *moving;
    int index = _index;

    if ( (old_parent->id == _parent) &&
         (index > old_index) )
    {
        --index;
    }

    (void)remove_row(_sheet, _id);

    row* destination = find_row(_sheet, _parent);

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) > destination->children.size()) )
    {
        destination->children.push_back(std::move(copy));
    }
    else
    {
        destination->children.insert(destination->children.begin() + index,
                                     std::move(copy));
    }

    return true;
}


// --------------------------------------------------------------- allocation

int
next_free_bit(
    const sheet& _sheet,
    int          _group
)
{
    const row* group = find_row(_sheet, _group);
    wide       used;

    // a symbol stands for the group it is in
    if ( (group != nullptr) &&
         (!can_contain(*group)) )
    {
        sheet&     mutable_sheet = const_cast<sheet&>(_sheet);
        const row* parent        = parent_of(mutable_sheet, group->id);
        group = parent;
    }

    if (group == nullptr)
    {
        return 0;
    }

    for (const row& child : group->children)
    {
        if (child.kind != row_kind::symbol)
        {
            continue;
        }

        if (child.def == def_kind::bit)
        {
            used |= wide::bit(child.bit);
        }
        else if (child.def == def_kind::range)
        {
            used |= wide::run(child.lsb, child.width);
        }
    }

    // Respect the tightest enclosing range, even when there are
    // ordinary groups between it and the selected node.
    int low = 0;
    int end = _sheet.width;
    const row* ancestor = group;
    while (ancestor != nullptr)
    {
        if ((ancestor->kind == row_kind::symbol) &&
            (ancestor->def == def_kind::range))
        {
            low = std::max(low, ancestor->lsb);
            end = std::min(end, ancestor->lsb + ancestor->width);
        }
        ancestor = parent_of(const_cast<sheet&>(_sheet), ancestor->id);
    }
    for (int i = std::max(0, low); i < end; ++i)
    {
        if (!used.test(i))
        {
            return i;
        }
    }

    return -1;
}

bool
function_like(
    const row& _row
)
{
    return ( (_row.callable) ||
             (!_row.params.empty()) );
}

/*
host_data_model
  Read from the compiler that built bitwisedom: a 64-bit long is LP64, a
64-bit pointer beside a 32-bit long is LLP64, and anything else is taken
for a 32-bit target.
*/
data_model
host_data_model()
{
    if constexpr (sizeof(long) == 8)
    {
        return data_model::lp64;
    }
    else if constexpr (sizeof(void*) == 8)
    {
        return data_model::llp64;
    }
    else
    {
        return data_model::ilp32;
    }
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
