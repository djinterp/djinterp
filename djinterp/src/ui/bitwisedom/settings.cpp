/*******************************************************************************
* bitwisedom [ui]                                                   settings.cpp
*
* The settings registries, and settings files.
*   A sheet's settings are the model's, each given a write that takes a
* payload. The window's own are made here by a small factory per kind, from
* a function that reaches the field in the shell. One reach serves both
* directions: reading takes the field, writing assigns it, so the two
* cannot disagree on which field a path names.
*   A settings file is JSON: its format and version, then the window's own
* settings and a sheet's preferences, each an object of values by path --
* a flag as a boolean, a choice or an integer as a number, a real as a
* number, a colour as #RRGGBBAA and text as a string. Reading it gives back
* the payloads a "set" of each would post, so a file is applied exactly as
* the controls would apply it.
*
* path:      /src/ui/bitwisedom/settings.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.22
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/settings.hpp"  // corresponding header

// std
#include <algorithm>  // std::clamp
#include <cmath>      // std::lround
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/json.hpp>  // json_value


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::option_set;

/*
windowed
  The model's setting, given the write the window posts to: the payload's
"value" -- or, for a colour, its "r", "g", "b" and "a" -- in the form the
model's write takes, each part the payload leaves out the setting's own,
as it was.
*/
setting
windowed(
    const sheet_setting& _model
)
{
    setting made;

    made.path      = _model.path;
    made.label     = _model.label;
    made.kind      = _model.kind;
    made.low       = _model.low;
    made.high      = _model.high;
    made.choices   = _model.choices;
    made.help      = _model.help;
    made.read      = _model.read;
    made.read_text = _model.read_text;

    switch (_model.kind)
    {
        case setting_kind::flag:
            made.write = [&_model](sheet& _sheet, const option_set& _payload)
            {
                const bool held = (_model.read(_sheet) != 0.0);

                _model.write(_sheet,
                             _payload.as_bool("value", held) ? 1.0 : 0.0);
            };
            break;

        case setting_kind::integer:
        case setting_kind::choice:
            made.write = [&_model](sheet& _sheet, const option_set& _payload)
            {
                const long held = std::lround(_model.read(_sheet));

                _model.write(_sheet,
                             static_cast<double>(
                                 _payload.as_long("value", held)));
            };
            break;

        case setting_kind::real:
            made.write = [&_model](sheet& _sheet, const option_set& _payload)
            {
                _model.write(_sheet,
                             _payload.as_double("value",
                                                _model.read(_sheet)));
            };
            break;

        case setting_kind::colour:
            made.write = [&_model](sheet& _sheet, const option_set& _payload)
            {
                const rgba held = static_cast<rgba>(_model.read(_sheet));
                const rgba made_colour = make_rgba(
                    static_cast<float>(_payload.as_double("r",
                                                          channel(held, 0))),
                    static_cast<float>(_payload.as_double("g",
                                                          channel(held, 1))),
                    static_cast<float>(_payload.as_double("b",
                                                          channel(held, 2))),
                    static_cast<float>(_payload.as_double("a",
                                                          channel(held, 3))));

                _model.write(_sheet,
                             static_cast<double>(made_colour));
            };
            break;

        case setting_kind::text:
            made.write = [&_model](sheet& _sheet, const option_set& _payload)
            {
                _model.write_text(_sheet,
                                  _payload.as_string("value",
                                                     _model.read_text(
                                                         _sheet)));
            };
            break;
    }

    return made;
}

// make_settings
//   the window's table: the model's, entry for entry, in its order.
std::vector<setting>
make_settings()
{
    std::vector<setting> out;

    // the model's entries live as long as the program, so a write may keep
    // a reference to its own
    for (const sheet_setting& each : sheet_settings())
    {
        out.push_back(windowed(each));
    }

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
        "environment.tabs_over", "The sheets' tabs run over",
        { "every column", "the bits", "the symbols column",
          "the value column" },
        "the row of a sheet's tabs: across the whole width, as it always "
        "was; or over one of the sheet's columns, as Visual Studio's code "
        "tabs run over the code -- the rest of the row the grid's, up to "
        "the window's top. A column hidden is every column",
        [](const shell_state& _s)
        {
            return static_cast<int>(_s.prefs.tabs_over);
        },
        [](shell_state& _s, int _index)
        {
            _s.prefs.tabs_over = static_cast<tab_row>(_index);
        }));
    out.push_back(env_choice(
        "environment.tabs_align", "The sheets' tabs keep to",
        { "the left", "the right" },
        "which end of what they run over the sheets' tabs keep to",
        [](const shell_state& _s) { return _s.prefs.tabs_right ? 1 : 0; },
        [](shell_state& _s, int _index)
        {
            _s.prefs.tabs_right = (_index == 1);
        }));
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
    return bitwisedom::is_preference(_setting.path);
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
