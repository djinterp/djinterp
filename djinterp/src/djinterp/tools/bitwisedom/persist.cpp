/*******************************************************************************
* bitwisedom [bitwisedom]                                            persist.cpp
*
* Saving and loading sheets.
*   Each enumeration has one table of words, used both ways, so a value
* written is always a value read. Two enumerations count from -1 (inherit);
* their tables carry an offset.
*
* path:      /src/djinterp/tools/bitwisedom/persist.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include <djinterp/tools/bitwisedom/persist.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>         // std::max
#include <cmath>             // std::fabs
#include <cstddef>           // std::size_t
#include <filesystem>        // std::filesystem::path
#include <fstream>           // std::ifstream, std::ofstream
#include <initializer_list>  // std::initializer_list
#include <iterator>          // std::istreambuf_iterator
#include <utility>           // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/json.hpp>  // json_value, parse_json


namespace bitwisedom
{

namespace
{

// words
//   struct: one enumeration's words, and the value its first word means.
struct words
{
    std::vector<const char*> names;
    int                      first = 0;
};

const words ROW_KINDS  = { { "group", "symbol" }, 0 };
const words DEF_KINDS  = { { "bit", "range", "field_value", "literal",
                             "expression", "macro" }, 0 };
const words ENCODINGS  = { { "unsigned", "twos_complement", "ones_complement",
                             "sign_magnitude", "offset_binary" }, 0 };
const words RADIXES    = { { "inherit", "hex", "dec", "oct", "bin",
                             "shift" }, -1 };
const words SUFFIXES   = { { "inherit", "none", "u", "ul", "ull" }, -1 };
const words NUMBERINGS = { { "lsb0", "msb0" }, 0 };
const words ORDERS     = { { "little", "big" }, 0 };
const words STYLES     = { { "solid", "dashed", "dotted" }, 0 };
const words COLLAPSES  = { { "hex", "dots", "count", "blank" }, 0 };
const words MODES      = { { "row", "group", "kind", "accent" }, 0 };
const words FILLS      = { { "cells", "names", "none" }, 0 };
const words VALUE_FORMS = { { "written", "hex", "dec", "bin", "shift",
                              "bits" }, 0 };
const words SORT_KEYS  = { { "sheet", "symbol", "name", "short_name", "value",
                             "lowest", "width", "kind", "note" }, 0 };
const words LANGUAGES  = { { "c", "cpp" }, 0 };
const words FORMS      = { { "macros", "constants" }, 0 };
const words GEN_STYLES = { { "djinterp", "plain" }, 0 };
const words STANDARDS  = { { "c99", "c11", "c17", "c23", "cpp98", "cpp11",
                             "cpp14", "cpp17", "cpp20", "cpp23" }, 0 };
const words MODELS     = { { "lp64", "llp64", "ilp32" }, 0 };
const words SPECIALS   = { { "probe", "usage" }, 0 };
const words PLACES     = { { "bottom", "top", "tree" }, 0 };

// word
//   an enumeration's value as its word.
template<typename Enum>
json_value
word(
    const words& _table,
    Enum         _value
)
{
    const int index = static_cast<int>(_value) - _table.first;

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= _table.names.size()) )
    {
        return json_value::string(_table.names.front());
    }

    return json_value::string(_table.names[static_cast<std::size_t>(index)]);
}

// read_word
//   a word as an enumeration's value; the fallback for anything else.
template<typename Enum>
Enum
read_word(
    const json_value* _value,
    const words&      _table,
    Enum              _fallback
)
{
    if (_value == nullptr)
    {
        return _fallback;
    }

    const std::string text = _value->as_string("");

    for (std::size_t i = 0; i < _table.names.size(); ++i)
    {
        if (text == _table.names[i])
        {
            return static_cast<Enum>(static_cast<int>(i) + _table.first);
        }
    }

    return _fallback;
}

json_value
hex_value(
    const wide& _value
)
{
    return json_value::string("0x" + format_hex(_value, 0, true));
}

/*
read_hex
  A value of any width, as 0x-prefixed text -- or any base parse_wide
reads. A negative number is its 64-bit two's complement, as a literal's
bits were always written.
*/
wide
read_hex(
    const json_value* _value,
    const wide&       _fallback
)
{
    if (_value == nullptr)
    {
        return _fallback;
    }

    const parsed_wide parsed = parse_wide(_value->as_string(""));

    return parsed.ok ? parsed.value
                     : _fallback;
}

json_value
colour_value(
    rgba _colour
)
{
    return json_value::string(rgba_text(_colour));
}

rgba
read_colour(
    const json_value* _value,
    rgba              _fallback
)
{
    rgba colour = _fallback;

    if ( (_value != nullptr) &&
         (!parse_rgba(_value->as_string(""), colour)) )
    {
        return _fallback;
    }

    return colour;
}

// field readers -- each takes a member of an object, with a fallback
bool
get_bool(
    const json_value&  _object,
    const std::string& _key,
    bool               _fallback
)
{
    const json_value* found = _object.find(_key);

    return (found != nullptr) ? found->as_bool(_fallback)
                              : _fallback;
}

i64
get_int(
    const json_value&  _object,
    const std::string& _key,
    i64                _fallback
)
{
    const json_value* found = _object.find(_key);

    return (found != nullptr) ? found->as_int(_fallback)
                              : _fallback;
}

int
get_small(
    const json_value&  _object,
    const std::string& _key,
    int                _fallback
)
{
    const i64 value = get_int(_object, _key, _fallback);

    // a count or a position: anything wild keeps the default
    if ( (value < -1000000) ||
         (value > 1000000) )
    {
        return _fallback;
    }

    return static_cast<int>(value);
}

float
get_float(
    const json_value&  _object,
    const std::string& _key,
    float              _fallback
)
{
    const json_value* found = _object.find(_key);

    return (found != nullptr)
               ? static_cast<float>(found->as_real(_fallback))
               : _fallback;
}

std::string
get_text(
    const json_value&  _object,
    const std::string& _key,
    const std::string& _fallback
)
{
    const json_value* found = _object.find(_key);

    return (found != nullptr) ? found->as_string(_fallback)
                              : _fallback;
}

// --------------------------------------------------------------------- rows

json_value
save_row(
    const row& _row
)
{
    json_value out = json_value::object();

    out.set("id", json_value::number(_row.id));
    out.set("kind", word(ROW_KINDS, _row.kind));
    out.set("name", json_value::string(_row.name));
    out.set("short", json_value::string(_row.short_name));
    out.set("note", json_value::string(_row.note));
    out.set("colour", colour_value(_row.colour));
    out.set("transparent", json_value::boolean(_row.transparent));
    out.set("emit", json_value::boolean(_row.emit));
    out.set("prefix", json_value::string(_row.prefix));
    out.set("suffix", json_value::string(_row.suffix));
    out.set("radix", word(RADIXES, _row.fmt));
    out.set("int_suffix", word(SUFFIXES, _row.suf));

    if (can_contain(_row))
    {
        out.set("collapsed", json_value::boolean(_row.collapsed));
        out.set("isolate", json_value::boolean(_row.isolate));
        out.set("name_children", json_value::boolean(_row.name_children));
        out.set("exclusive", json_value::boolean(_row.exclusive));

        json_value children = json_value::array();

        for (const row& child : _row.children)
        {
            children.push(save_row(child));
        }

        out.set("rows", std::move(children));

        if (_row.kind == row_kind::group)
        {
            return out;
        }
    }

    // every definition field is kept, so switching kinds loses nothing
    out.set("def", word(DEF_KINDS, _row.def));
    out.set("bit", json_value::number(_row.bit));
    out.set("lsb", json_value::number(_row.lsb));
    out.set("width", json_value::number(_row.width));
    out.set("field", json_value::number(_row.field));
    out.set("encoding", word(ENCODINGS, _row.enc));
    out.set("bias", json_value::number(_row.bias));
    out.set("number", json_value::number(_row.number));
    out.set("literal", hex_value(_row.literal));
    out.set("negative", json_value::boolean(_row.negative));
    out.set("digits", json_value::number(_row.digits));
    out.set("expr", json_value::string(_row.expr));
    out.set("params", json_value::string(_row.params));
    out.set("body", json_value::string(_row.body));
    out.set("callable", json_value::boolean(_row.callable));

    return out;
}

row
load_row(
    const json_value& _value,
    int&              _top
)
{
    row made;

    made.id         = get_small(_value, "id", 0);
    made.kind       = read_word(_value.find("kind"), ROW_KINDS,
                                row_kind::symbol);
    made.name       = get_text(_value, "name", "");
    made.short_name = get_text(_value, "short", "");
    made.note       = get_text(_value, "note", "");
    made.colour     = read_colour(_value.find("colour"), 0);
    made.transparent = get_bool(_value, "transparent", false);
    made.emit       = get_bool(_value, "emit", true);
    made.prefix     = get_text(_value, "prefix", "");
    made.suffix     = get_text(_value, "suffix", "");
    made.fmt        = read_word(_value.find("radix"), RADIXES,
                                radix::inherit);
    made.suf        = read_word(_value.find("int_suffix"), SUFFIXES,
                                int_suffix::inherit);
    made.collapsed  = get_bool(_value, "collapsed", false);
    made.isolate    = get_bool(_value, "isolate", false);
    made.name_children = get_bool(_value, "name_children", false);
    made.exclusive  = get_bool(_value, "exclusive", true);
    made.def        = read_word(_value.find("def"), DEF_KINDS,
                                def_kind::bit);
    made.bit        = get_small(_value, "bit", 0);
    made.lsb        = get_small(_value, "lsb", 0);
    made.width      = get_small(_value, "width", 1);
    made.field      = get_small(_value, "field", 0);
    made.enc        = read_word(_value.find("encoding"), ENCODINGS,
                                encoding::unsigned_binary);
    made.bias       = get_int(_value, "bias", 0);
    made.number     = get_int(_value, "number", 0);
    made.literal    = read_hex(_value.find("literal"), wide());
    made.negative   = get_bool(_value, "negative", false);
    made.digits     = get_small(_value, "digits", 0);
    made.expr       = get_text(_value, "expr", "");
    made.params     = get_text(_value, "params", "");
    made.body       = get_text(_value, "body", "");
    made.callable   = get_bool(_value, "callable", false);

    _top = std::max(_top, made.id);

    const json_value* children = _value.find("rows");

    if ( (children != nullptr) &&
         (can_contain(made)) )
    {
        for (const json_value& child : children->items)
        {
            made.children.push_back(load_row(child, _top));
        }
    }

    return made;
}

// ---------------------------------------------------------------- settings

json_value
save_line(
    const line_spec& _line
)
{
    json_value out = json_value::object();

    out.set("show", json_value::boolean(_line.show));
    out.set("thickness", json_value::real(_line.thickness));

    // an automatic colour is left out, and stays automatic when read
    if (_line.colour != 0)
    {
        out.set("colour", colour_value(_line.colour));
    }

    out.set("style", word(STYLES, _line.style));

    return out;
}

line_spec
load_line(
    const json_value* _value,
    const line_spec&  _fallback
)
{
    if (_value == nullptr)
    {
        return _fallback;
    }

    line_spec made;
    made.show      = get_bool(*_value, "show", _fallback.show);
    made.thickness = get_float(*_value, "thickness", _fallback.thickness);
    made.colour    = read_colour(_value->find("colour"), _fallback.colour);
    made.style     = read_word(_value->find("style"), STYLES,
                               _fallback.style);

    return made;
}

json_value
save_grid(
    const grid_settings& _grid
)
{
    json_value out = json_value::object();

    out.set("lines", json_value::boolean(_grid.lines));
    out.set("minor", save_line(_grid.minor));
    out.set("major", save_line(_grid.major));
    out.set("rows", save_line(_grid.rows));
    out.set("groups", save_line(_grid.groups));
    out.set("segments", save_line(_grid.segments));
    out.set("border", save_line(_grid.border));
    out.set("major_every", json_value::number(_grid.major_every));
    out.set("cell_width", json_value::real(_grid.cell_width));
    out.set("cell_height", json_value::real(_grid.cell_height));
    out.set("cell_gap", json_value::real(_grid.cell_gap));
    out.set("rounding", json_value::real(_grid.rounding));
    out.set("name_width", json_value::real(_grid.name_width));
    out.set("value_width", json_value::real(_grid.value_width));
    out.set("indent", json_value::real(_grid.indent));
    out.set("index_every", json_value::number(_grid.index_every));
    out.set("auto_segments", json_value::number(_grid.auto_segments));
    out.set("show_indices", json_value::boolean(_grid.show_indices));
    out.set("show_digits", json_value::boolean(_grid.show_digits));
    out.set("show_probe", json_value::boolean(_grid.show_probe));
    out.set("show_usage", json_value::boolean(_grid.show_usage));
    out.set("lines_past_rows", json_value::boolean(_grid.lines_past_rows));
    out.set("show_labels", json_value::boolean(_grid.show_labels));
    out.set("show_colours", json_value::boolean(_grid.show_colours));
    out.set("show_names", json_value::boolean(_grid.show_names));
    out.set("show_values", json_value::boolean(_grid.show_values));
    out.set("show_bits", json_value::boolean(_grid.show_bits));
    out.set("values_first", json_value::boolean(_grid.values_first));
    out.set("show_cell_boxes", json_value::boolean(_grid.show_cell_boxes));
    out.set("square_cells", json_value::boolean(_grid.square_cells));
    out.set("cell_ratio", json_value::real(_grid.cell_ratio));
    out.set("full_names", json_value::boolean(_grid.full_names));
    out.set("names_by_value", json_value::boolean(_grid.names_by_value));
    out.set("names_value_form",
            word(VALUE_FORMS, _grid.names_value_form));
    out.set("abbreviate_unused",
            json_value::boolean(_grid.abbreviate_unused));
    out.set("unused_run", json_value::number(_grid.unused_run));
    out.set("collapsed", word(COLLAPSES, _grid.collapsed));
    out.set("fill", word(FILLS, _grid.fill));
    out.set("sort_by", word(SORT_KEYS, _grid.sort_by));
    out.set("sort_descending", json_value::boolean(_grid.sort_descending));

    // the regions that show their cells otherwise, each with only what it
    // changes
    if (!_grid.rules.empty())
    {
        json_value rules = json_value::array();

        for (const display_rule& each : _grid.rules)
        {
            json_value rule = json_value::object();
            json_value ids  = json_value::array();

            for (int id : each.rows)
            {
                ids.push(json_value::number(id));
            }

            rule.set("rows", std::move(ids));
            rule.set("lsb", json_value::number(each.lsb));
            rule.set("msb", json_value::number(each.msb));

            for (const auto& part : { std::make_pair("digits", each.digits),
                                      std::make_pair("names", each.names),
                                      std::make_pair("colour",
                                                     each.colour) })
            {
                if (part.second >= 0)
                {
                    rule.set(part.first, json_value::boolean(part.second != 0));
                }
            }

            rules.push(std::move(rule));
        }

        out.set("rules", std::move(rules));
    }

    // the special rows, in their order: where each is shown, and among the
    // rows, the row it comes before or the group it ends
    json_value specials = json_value::array();

    for (const special_row& each : _grid.specials)
    {
        json_value one = json_value::object();

        one.set("kind", word(SPECIALS, each.kind));
        one.set("place", word(PLACES, each.place));

        if (each.place == special_place::tree)
        {
            one.set("parent", json_value::number(each.parent));
            one.set("before", json_value::number(each.before));
        }

        specials.push(std::move(one));
    }

    out.set("specials", std::move(specials));

    return out;
}

void
load_grid(
    const json_value* _value,
    grid_settings&    _grid
)
{
    if (_value == nullptr)
    {
        return;
    }

    const json_value& in = *_value;

    _grid.lines       = get_bool(in, "lines", _grid.lines);
    _grid.minor       = load_line(in.find("minor"), _grid.minor);
    _grid.major       = load_line(in.find("major"), _grid.major);
    _grid.rows        = load_line(in.find("rows"), _grid.rows);
    _grid.groups      = load_line(in.find("groups"), _grid.groups);
    _grid.segments    = load_line(in.find("segments"), _grid.segments);
    _grid.border      = load_line(in.find("border"), _grid.border);
    _grid.major_every = get_small(in, "major_every", _grid.major_every);
    _grid.cell_width  = get_float(in, "cell_width", _grid.cell_width);
    _grid.cell_height = get_float(in, "cell_height", _grid.cell_height);
    _grid.cell_gap    = get_float(in, "cell_gap", _grid.cell_gap);
    _grid.rounding    = get_float(in, "rounding", _grid.rounding);
    _grid.name_width  = get_float(in, "name_width", _grid.name_width);
    _grid.value_width = get_float(in, "value_width", _grid.value_width);
    _grid.indent      = get_float(in, "indent", _grid.indent);
    _grid.index_every = get_small(in, "index_every", _grid.index_every);
    _grid.auto_segments =
        get_small(in, "auto_segments", _grid.auto_segments);
    _grid.show_indices = get_bool(in, "show_indices", _grid.show_indices);
    _grid.show_digits  = get_bool(in, "show_digits", _grid.show_digits);
    _grid.show_probe   = get_bool(in, "show_probe", _grid.show_probe);
    _grid.show_usage   = get_bool(in, "show_usage", _grid.show_usage);
    _grid.lines_past_rows =
        get_bool(in, "lines_past_rows", _grid.lines_past_rows);
    _grid.show_labels  = get_bool(in, "show_labels", _grid.show_labels);
    _grid.show_colours = get_bool(in, "show_colours", _grid.show_colours);
    _grid.show_names = get_bool(in, "show_names", _grid.show_names);
    _grid.show_values = get_bool(in, "show_values", _grid.show_values);
    _grid.show_bits = get_bool(in, "show_bits", _grid.show_bits);
    _grid.values_first = get_bool(in, "values_first", _grid.values_first);
    _grid.show_cell_boxes = get_bool(
        in, "show_cell_boxes",
        get_bool(in, "show_range_boxes", _grid.show_cell_boxes));
    _grid.square_cells = get_bool(in, "square_cells", _grid.square_cells);
    _grid.cell_ratio   = get_float(in, "cell_ratio", _grid.cell_ratio);
    _grid.full_names   = get_bool(in, "full_names", _grid.full_names);
    _grid.names_by_value =
        get_bool(in, "names_by_value", _grid.names_by_value);
    _grid.names_value_form = read_word(in.find("names_value_form"),
                                       VALUE_FORMS,
                                       _grid.names_value_form);
    _grid.abbreviate_unused =
        get_bool(in, "abbreviate_unused", _grid.abbreviate_unused);
    _grid.unused_run   = get_small(in, "unused_run", _grid.unused_run);
    _grid.collapsed    = read_word(in.find("collapsed"), COLLAPSES,
                                   _grid.collapsed);
    _grid.fill         = read_word(in.find("fill"), FILLS, _grid.fill);
    _grid.sort_by      = read_word(in.find("sort_by"), SORT_KEYS,
                                   _grid.sort_by);
    _grid.sort_descending =
        get_bool(in, "sort_descending", _grid.sort_descending);

    const json_value* rules = in.find("rules");

    if ( (rules != nullptr) &&
         (rules->type == json_type::array) )
    {
        _grid.rules.clear();

        for (const json_value& each : rules->items)
        {
            display_rule      made;
            const json_value* ids = each.find("rows");

            if ( (ids != nullptr) &&
                 (ids->type == json_type::array) )
            {
                for (const json_value& id : ids->items)
                {
                    made.rows.push_back(static_cast<int>(id.as_int(0)));
                }
            }

            made.lsb = static_cast<int>(get_int(each, "lsb", 0));
            made.msb = static_cast<int>(get_int(each, "msb", -1));

            // tri
            //   a part the rule sets, or -1 for one it leaves the sheet's.
            auto tri = [&each](const char* _key)
            {
                const json_value* found = each.find(_key);

                return (found == nullptr) ? -1
                                          : (found->as_bool(false) ? 1 : 0);
            };

            made.digits = tri("digits");
            made.names  = tri("names");
            made.colour = tri("colour");
            _grid.rules.push_back(std::move(made));
        }
    }

    // the special rows; a file from before them has the probe and the
    // usage pinned at the bottom, as the sheet it is read into has
    const json_value* specials = in.find("specials");

    if ( (specials != nullptr) &&
         (specials->type == json_type::array) )
    {
        _grid.specials.clear();

        for (const json_value& each : specials->items)
        {
            special_row made;

            made.id     = static_cast<int>(_grid.specials.size()) + 1;
            made.kind   = read_word(each.find("kind"), SPECIALS, made.kind);
            made.place  = read_word(each.find("place"), PLACES, made.place);
            made.parent = static_cast<int>(get_int(each, "parent", 0));
            made.before = static_cast<int>(get_int(each, "before", 0));
            _grid.specials.push_back(made);
        }
    }

    return;
}

/*
save_colours
  Only what the sheet sets: an automatic colour, palette, saturation or
brightness is left out, and is automatic again when read. The theme is the
window's, and is never written.
*/
json_value
save_colours(
    const colour_settings& _colours
)
{
    json_value out = json_value::object();

    // set
    //   one role's colour, if the sheet sets it.
    auto set = [&out](const char* _key,
                      rgba        _colour)
    {
        if (_colour != 0)
        {
            out.set(_key, colour_value(_colour));
        }

        return;
    };

    if (_colours.palette >= 0)
    {
        out.set("palette", json_value::number(_colours.palette));
    }

    if (_colours.saturation >= 0.0f)
    {
        out.set("saturation", json_value::real(_colours.saturation));
    }

    if (_colours.value >= 0.0f)
    {
        out.set("value", json_value::real(_colours.value));
    }

    out.set("mode", word(MODES, _colours.mode));
    out.set("set_alpha", json_value::real(_colours.set_alpha));
    out.set("range_alpha", json_value::real(_colours.range_alpha));
    set("accent", _colours.accent);
    set("empty_cell", _colours.empty_cell);
    set("collision", _colours.collision);
    set("probe", _colours.probe);
    set("selection", _colours.selection);
    set("hover", _colours.hover);
    set("ghost", _colours.ghost);

    return out;
}

/*
forget_old_defaults
  A sheet saved before themes and schemes were the window's -- one whose
colours name a "theme" -- wrote every colour out, the defaults with the
rest. A colour that is one of those defaults, any old theme's, was never
chosen: it becomes automatic, so the sheet follows the window's theme and
scheme. So do the palette, saturation and brightness the old themes gave.
*/
void
forget_old_defaults(
    sheet& _sheet
)
{
    colour_settings& c = _sheet.colours;
    grid_settings&   g = _sheet.grid;

    // forget
    //   a colour, if it is one of the defaults listed.
    auto forget = [](rgba&                         _colour,
                     std::initializer_list<rgba>   _defaults)
    {
        for (rgba each : _defaults)
        {
            if (_colour == each)
            {
                _colour = 0;
            }
        }

        return;
    };

    // dark, light and paper, midnight
    forget(c.accent, { 0x4C8DFFFFu, 0x2F6FE0FFu, 0x7AA2F7FFu });
    forget(c.empty_cell, { 0x2A2E36FFu, 0xEEF0F4FFu, 0x161A24FFu });
    forget(c.collision, { 0xFF4D4DFFu });
    forget(c.probe, { 0xFFC247FFu });
    forget(c.selection, { 0x4C8DFF40u, 0x2F6FE030u });
    forget(c.hover, { 0xFFFFFF1Cu, 0x0000001Au });
    forget(c.ghost, { 0xFFFFFFC0u, 0x202020C0u });
    forget(g.minor.colour, { 0x3B4252FFu, 0xC9CED8FFu, 0x262C3AFFu });
    forget(g.major.colour, { 0x7B8499FFu, 0x7A8294FFu });
    forget(g.rows.colour, { 0x2E333DFFu, 0xDDE1E8FFu, 0x1F2430FFu });
    forget(g.groups.colour, { 0x5A6275FFu, 0x9AA1B0FFu });
    forget(g.segments.colour, { 0x4C8DFFA0u, 0x2F6FE0B0u });
    forget(g.border.colour, { 0x5A6275FFu, 0x9AA1B0FFu });

    if (c.palette == 0)
    {
        c.palette = -1;
    }

    // near
    //   whether a saved real is the default it was written from.
    auto near = [](float _value,
                   float _default)
    {
        return (std::fabs(_value - _default) < 0.0005f);
    };

    if ( ( (near(c.saturation, 0.62f)) && (near(c.value, 0.92f)) ) ||
         ( (near(c.saturation, 0.70f)) && (near(c.value, 0.82f)) ) )
    {
        c.saturation = -1.0f;
        c.value      = -1.0f;
    }

    return;
}

void
load_colours(
    const json_value* _value,
    colour_settings&  _colours
)
{
    if (_value == nullptr)
    {
        return;
    }

    const json_value& in = *_value;

    _colours.palette     = get_small(in, "palette", _colours.palette);
    _colours.saturation  = get_float(in, "saturation", _colours.saturation);
    _colours.value       = get_float(in, "value", _colours.value);
    _colours.mode        = read_word(in.find("mode"), MODES, _colours.mode);
    _colours.set_alpha   = get_float(in, "set_alpha", _colours.set_alpha);
    _colours.range_alpha = get_float(in, "range_alpha", _colours.range_alpha);
    _colours.accent      = read_colour(in.find("accent"), _colours.accent);
    _colours.empty_cell  = read_colour(in.find("empty_cell"),
                                       _colours.empty_cell);
    _colours.collision   = read_colour(in.find("collision"),
                                       _colours.collision);
    _colours.probe       = read_colour(in.find("probe"), _colours.probe);
    _colours.selection   = read_colour(in.find("selection"),
                                       _colours.selection);
    _colours.hover       = read_colour(in.find("hover"), _colours.hover);
    _colours.ghost       = read_colour(in.find("ghost"), _colours.ghost);

    return;
}

json_value
save_gen(
    const gen_settings& _gen
)
{
    json_value out = json_value::object();

    out.set("language", word(LANGUAGES, _gen.lang));
    out.set("form", word(FORMS, _gen.form));
    out.set("style", word(GEN_STYLES, _gen.style));
    out.set("level", word(STANDARDS, _gen.level));
    out.set("radix", word(RADIXES, _gen.base));
    out.set("int_suffix", word(SUFFIXES, _gen.suffix));
    out.set("upper_hex", json_value::boolean(_gen.upper_hex));
    out.set("upper_suffix", json_value::boolean(_gen.upper_suffix));
    out.set("pad", json_value::boolean(_gen.pad));
    out.set("banner", json_value::boolean(_gen.banner));
    out.set("guard", json_value::boolean(_gen.guard));
    out.set("pragma_once", json_value::boolean(_gen.pragma_once));
    out.set("comments", json_value::boolean(_gen.comments));
    out.set("sections", json_value::boolean(_gen.sections));
    out.set("align", json_value::boolean(_gen.align));
    out.set("file_name", json_value::string(_gen.file_name));
    out.set("guard_name", json_value::string(_gen.guard_name));
    out.set("namespace", json_value::string(_gen.name_space));
    out.set("author", json_value::string(_gen.author));
    out.set("data_model", word(MODELS, _gen.model));
    out.set("word", json_value::number(_gen.word));

    json_value templates = json_value::array();

    for (const macro_template& each : _gen.templates)
    {
        json_value made = json_value::object();

        made.set("enabled", json_value::boolean(each.enabled));
        made.set("applies", json_value::number(each.applies));
        made.set("name", json_value::string(each.name));
        made.set("params", json_value::string(each.params));
        made.set("body", json_value::string(each.body));
        made.set("note", json_value::string(each.note));

        if (each.wide)
        {
            made.set("wide", json_value::boolean(true));
        }

        templates.push(std::move(made));
    }

    out.set("templates", std::move(templates));

    return out;
}

void
load_gen(
    const json_value* _value,
    gen_settings&     _gen
)
{
    if (_value == nullptr)
    {
        return;
    }

    const json_value& in = *_value;

    _gen.lang         = read_word(in.find("language"), LANGUAGES, _gen.lang);
    _gen.form         = read_word(in.find("form"), FORMS, _gen.form);
    _gen.style        = read_word(in.find("style"), GEN_STYLES, _gen.style);
    _gen.level        = read_word(in.find("level"), STANDARDS, _gen.level);
    _gen.base         = read_word(in.find("radix"), RADIXES, _gen.base);
    _gen.suffix       = read_word(in.find("int_suffix"), SUFFIXES,
                                  _gen.suffix);
    _gen.upper_hex    = get_bool(in, "upper_hex", _gen.upper_hex);
    _gen.upper_suffix = get_bool(in, "upper_suffix", _gen.upper_suffix);
    _gen.pad          = get_bool(in, "pad", _gen.pad);
    _gen.banner       = get_bool(in, "banner", _gen.banner);
    _gen.guard        = get_bool(in, "guard", _gen.guard);
    _gen.pragma_once  = get_bool(in, "pragma_once", _gen.pragma_once);
    _gen.comments     = get_bool(in, "comments", _gen.comments);
    _gen.sections     = get_bool(in, "sections", _gen.sections);
    _gen.align        = get_bool(in, "align", _gen.align);
    _gen.file_name    = get_text(in, "file_name", _gen.file_name);
    _gen.guard_name   = get_text(in, "guard_name", _gen.guard_name);
    _gen.name_space   = get_text(in, "namespace", _gen.name_space);
    _gen.author       = get_text(in, "author", _gen.author);
    _gen.model        = read_word(in.find("data_model"), MODELS, _gen.model);
    _gen.word         = get_small(in, "word", _gen.word);

    const json_value* templates = in.find("templates");

    if (templates != nullptr)
    {
        bool wide_ones = false;

        _gen.templates.clear();

        for (const json_value& each : templates->items)
        {
            macro_template made;
            made.enabled = get_bool(each, "enabled", true);
            made.applies = get_small(each, "applies", 0);
            made.name    = get_text(each, "name", "");
            made.params  = get_text(each, "params", "");
            made.body    = get_text(each, "body", "");
            made.note    = get_text(each, "note", "");
            made.wide    = get_bool(each, "wide", false);
            wide_ones    = ( (wide_ones) || (made.wide) );
            _gen.templates.push_back(std::move(made));
        }

        // a file from before wide sheets has none of theirs: the defaults
        if (!wide_ones)
        {
            for (macro_template& each : default_wide_templates())
            {
                _gen.templates.push_back(std::move(each));
            }
        }
    }

    return;
}

/*
file_path
  A file's name as the system means it. Names travel through the program
as UTF-8 -- typed into ImGui, or dropped on the window -- and a stream
opened by a narrow string would read one in the system's code page on
Windows; a path made from UTF-8 says which.
*/
std::filesystem::path
file_path(
    const std::string& _utf8
)
{
#if defined(__cpp_char8_t)
    return std::filesystem::path(std::u8string(_utf8.begin(), _utf8.end()));
#else
    return std::filesystem::u8path(_utf8);
#endif
}

}  // anonymous namespace


json_value
sheet_object(
    const sheet& _sheet
)
{
    json_value out = json_value::object();

    out.set("format", json_value::string(SHEET_FORMAT));
    out.set("version", json_value::number(SHEET_VERSION));
    out.set("title", json_value::string(_sheet.title));
    out.set("width", json_value::number(_sheet.width));
    out.set("numbering", word(NUMBERINGS, _sheet.bits));
    out.set("msb_left", json_value::boolean(_sheet.msb_left));
    out.set("byte_order", word(ORDERS, _sheet.bytes));
    out.set("prefix", json_value::string(_sheet.prefix));
    out.set("suffix", json_value::string(_sheet.suffix));
    out.set("trim_both", json_value::string(_sheet.trim_both));
    out.set("trim_left", json_value::string(_sheet.trim_left));
    out.set("trim_right", json_value::string(_sheet.trim_right));
    out.set("probe", hex_value(_sheet.probe));
    out.set("environment", json_value::string(_sheet.environment));
    json_value definitions = json_value::array();
    for (const definition_source& source : _sheet.definitions)
    {
        json_value entry = json_value::object();
        entry.set("path", json_value::string(source.path));
        entry.set("text", json_value::string(source.text));
        entry.set("show_other", json_value::boolean(source.show_other));
        definitions.items.push_back(std::move(entry));
    }
    out.set("definitions", std::move(definitions));

    json_value rows = json_value::array();

    for (const row& child : _sheet.root.children)
    {
        rows.push(save_row(child));
    }

    out.set("rows", std::move(rows));

    json_value segments = json_value::array();

    for (const column_segment& each : _sheet.segments)
    {
        json_value made = json_value::object();

        made.set("name", json_value::string(each.name));
        made.set("lsb", json_value::number(each.lsb));
        made.set("msb", json_value::number(each.msb));
        made.set("colour", colour_value(each.colour));
        made.set("collapsed", json_value::boolean(each.collapsed));
        segments.push(std::move(made));
    }

    out.set("segments", std::move(segments));
    out.set("grid", save_grid(_sheet.grid));
    out.set("colours", save_colours(_sheet.colours));
    out.set("generator", save_gen(_sheet.gen));

    return out;
}

std::string
save_sheet(
    const sheet& _sheet
)
{
    return write_json(sheet_object(_sheet));
}

bool
load_sheet(
    std::string_view _text,
    sheet&           _sheet,
    std::string&     _error
)
{
    json_value in;

    if (!parse_json(_text, in, _error))
    {
        return false;
    }

    return sheet_from_object(in, _sheet, _error);
}

bool
sheet_from_object(
    const json_value& _object,
    sheet&            _sheet,
    std::string&      _error
)
{
    const json_value& in = _object;

    if ( (in.type != json_type::object) ||
         (get_text(in, "format", "") != SHEET_FORMAT) )
    {
        _error = std::string("not a bitwisedom sheet: it has no \"format\": "
                             "\"") + SHEET_FORMAT + "\"";

        return false;
    }

    const i64 version = get_int(in, "version", 0);

    if (version > SHEET_VERSION)
    {
        _error = "the sheet is version " + format_signed(version) +
                 "; this build reads up to version " +
                 format_signed(SHEET_VERSION);

        return false;
    }

    sheet made = make_sheet(get_text(in, "title", "untitled"),
                            get_small(in, "width", 32));

    made.bits     = read_word(in.find("numbering"), NUMBERINGS, made.bits);
    made.msb_left = get_bool(in, "msb_left", made.msb_left);
    made.bytes    = read_word(in.find("byte_order"), ORDERS, made.bytes);
    made.prefix   = get_text(in, "prefix", "");
    made.suffix   = get_text(in, "suffix", "");
    made.trim_both  = get_text(in, "trim_both", "");
    made.trim_left  = get_text(in, "trim_left", "");
    made.trim_right = get_text(in, "trim_right", "");
    made.probe    = read_hex(in.find("probe"), wide());
    made.environment = get_text(in, "environment", "");
    if (const json_value* files = in.find("definitions"))
    {
        if (files->type == json_type::array)
        {
            for (const json_value& entry : files->items)
            {
                if (entry.type != json_type::object) continue;
                definition_source source;
                source.path = get_text(entry, "path", "");
                source.text = get_text(entry, "text", "");
                source.show_other = get_bool(entry, "show_other", false);
                made.definitions.push_back(std::move(source));
            }
        }
    }

    load_colours(in.find("colours"), made.colours);
    load_grid(in.find("grid"), made.grid);
    load_gen(in.find("generator"), made.gen);

    // a sheet from before themes were the window's
    const json_value* colours = in.find("colours");

    if ( (colours != nullptr) &&
         (colours->find("theme") != nullptr) )
    {
        forget_old_defaults(made);
    }

    int               top  = 0;
    const json_value* rows = in.find("rows");

    if (rows != nullptr)
    {
        for (const json_value& each : rows->items)
        {
            made.root.children.push_back(load_row(each, top));
        }
    }

    const json_value* segments = in.find("segments");

    if (segments != nullptr)
    {
        for (const json_value& each : segments->items)
        {
            column_segment segment;
            segment.name      = get_text(each, "name", "");
            segment.lsb       = get_small(each, "lsb", 0);
            segment.msb       = get_small(each, "msb", 0);
            segment.colour    = read_colour(each.find("colour"), 0);
            segment.collapsed = get_bool(each, "collapsed", false);
            made.segments.push_back(std::move(segment));
        }
    }

    made.next_id = top + 1;

    // the special rows anchored only to rows the sheet has
    (void)settle_specials(made);

    _sheet       = std::move(made);

    return true;
}

json_value
generator_object(
    const gen_settings& _gen
)
{
    return save_gen(_gen);
}

void
read_generator(
    const json_value* _object,
    gen_settings&     _gen
)
{
    load_gen(_object, _gen);

    return;
}

// enum_word
//   each enumeration a file names, as the word it writes.
std::string
enum_word(
    encoding _value
)
{
    return word(ENCODINGS, _value).text;
}

std::string
enum_word(
    radix _value
)
{
    return word(RADIXES, _value).text;
}

std::string
enum_word(
    int_suffix _value
)
{
    return word(SUFFIXES, _value).text;
}

std::string
enum_word(
    numbering _value
)
{
    return word(NUMBERINGS, _value).text;
}

std::string
enum_word(
    byte_order _value
)
{
    return word(ORDERS, _value).text;
}

std::string
enum_word(
    def_kind _value
)
{
    return word(DEF_KINDS, _value).text;
}

namespace
{

// read_any
//   a word as one enumeration's value, when the table has it.
template<typename Enum>
bool
read_any(
    const std::string& _word,
    const words&       _table,
    Enum&              _value
)
{
    for (std::size_t i = 0; i < _table.names.size(); ++i)
    {
        if (_word == _table.names[i])
        {
            _value = static_cast<Enum>(static_cast<int>(i) + _table.first);

            return true;
        }
    }

    return false;
}

}  // anonymous namespace

// read_enum
//   a word as each enumeration a file names; false, leaving the value, for
// a word the enumeration has not.
bool
read_enum(
    const std::string& _word,
    encoding&          _value
)
{
    return read_any(_word, ENCODINGS, _value);
}

bool
read_enum(
    const std::string& _word,
    radix&             _value
)
{
    return read_any(_word, RADIXES, _value);
}

bool
read_enum(
    const std::string& _word,
    int_suffix&        _value
)
{
    return read_any(_word, SUFFIXES, _value);
}

bool
read_enum(
    const std::string& _word,
    numbering&         _value
)
{
    return read_any(_word, NUMBERINGS, _value);
}

bool
read_enum(
    const std::string& _word,
    byte_order&        _value
)
{
    return read_any(_word, ORDERS, _value);
}

bool
read_enum(
    const std::string& _word,
    def_kind&          _value
)
{
    return read_any(_word, DEF_KINDS, _value);
}

bool
read_file(
    const std::string& _path,
    std::string&       _text,
    std::string&       _error
)
{
    std::ifstream in(file_path(_path), std::ios::binary);

    if (!in)
    {
        _error = "cannot open " + _path;

        return false;
    }

    _text.assign(std::istreambuf_iterator<char>(in),
                 std::istreambuf_iterator<char>());

    return true;
}

bool
write_file(
    const std::string& _path,
    const std::string& _text,
    std::string&       _error
)
{
    std::ofstream out(file_path(_path), std::ios::binary | std::ios::trunc);

    if (!out)
    {
        _error = "cannot write " + _path;

        return false;
    }

    out << _text;

    if (!out)
    {
        _error = "writing " + _path + " failed";

        return false;
    }

    return true;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
