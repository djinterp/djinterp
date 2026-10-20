/*******************************************************************************
* bitwisedom [app]                                                       app.cpp
*
* The bitwisedom window: the frame loop and the reducer.
*   apply() is a dispatch on the event's kind. Kinds are grouped by their
* first word -- "row.", "grid.", "ops.", "file." -- and each group's events
* carry the row, setting or segment they concern in "target", as uxoxo's
* elements pass it through, and the new value in "value".
*   The context menu and renames say where they were asked for in the
* kind's last word -- "context.tree", "rename.grid" -- since a row's
* "target" is its id. A rename goes through rename_row(), which rewrites
* every reference to the row; the inspector's name field reports only when
* its edit is done, so a rename is never made of a half-typed name.
*   The panels' sizes are remembered between runs in ImGui's settings file,
* beside its tables' column widths, by a settings handler of the window's
* own; what it reads and writes is kept apart from the window, which ImGui
* may outlive.
*
* path:      /src/djinterp/tools/bitwisedom/app/app.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./app.hpp"  // corresponding header

// std
#include <algorithm>     // std::find, std::remove, std::clamp, std::rotate,
                         // std::stable_sort
#include <cctype>        // std::tolower
#include <cstddef>       // std::size_t
#include <cstdio>        // std::snprintf
#include <cmath>         // std::isfinite
#include <cstdlib>       // std::getenv, std::strtod, std::strtof
#include <cstring>       // std::strcmp, std::strlen, std::strncmp
#include <ctime>         // std::time, std::localtime
#include <filesystem>    // std::filesystem::create_directories
#include <functional>    // std::function
#include <map>           // std::map
#include <system_error>  // std::error_code
#include <utility>       // std::move
#include <variant>       // std::get_if
// imgui
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>  // GetActiveWindow, WideCharToMultiByte
#include <commdlg.h>  // GetOpenFileNameW
#endif

#include <imgui.h>           // ImGui
#include <misc/cpp/imgui_stdlib.h>  // InputText with std::string
#include <imgui_internal.h>  // ImGuiSettingsHandler, AddSettingsHandler
// uxoxo
#include <uxoxo/realize.hpp>  // realize
// bitwisedom
#include <djinterp/tools/bitwisedom/cimport.hpp>   // import_code, merge_rows
#include <djinterp/tools/bitwisedom/expr.hpp>      // parse_expression, print_c
#include <djinterp/tools/bitwisedom/examples.hpp>  // make_example
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet, write_export
#include <djinterp/tools/bitwisedom/json.hpp>      // json_value, write_json
#include <djinterp/tools/bitwisedom/persist.hpp>   // load_sheet, save_sheet
#include <djinterp/tools/bitwisedom/rename.hpp>    // rename_row
#include "./elements.hpp"                  // register_renderers
#include "./fonts/fonts.hpp"               // the embedded fonts
#include "./settings.hpp"                  // find_setting
#include "./theme.hpp"                     // apply_style, mono_font
#include "./view.hpp"                      // build_ui


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::event;
using uxoxo::option_set;

// today
//   the local date, as the generator's banner wants it.
std::string
today()
{
    const std::time_t now   = std::time(nullptr);
    const std::tm*    local = std::localtime(&now);

    if (local == nullptr)
    {
        return "2026.01.01";
    }

    char text[48];

    std::snprintf(text,
                  sizeof(text),
                  "%04d.%02d.%02d",
                  local->tm_year + 1900,
                  local->tm_mon + 1,
                  local->tm_mday);

    return text;
}

// directory_of
//   the directory part of a path, with its separator; empty for none.
std::string
directory_of(
    const std::string& _path
)
{
    const std::size_t slash = _path.find_last_of("/\\");

    return (slash == std::string::npos) ? std::string()
                                        : _path.substr(0, slash + 1);
}

// as_number
//   a payload's number, posted as a long or as a double.
double
as_number(
    const option_set&  _payload,
    const std::string& _key,
    double             _fallback
)
{
    const ::djinterp::option_value* found = _payload.find(_key);

    if (found == nullptr)
    {
        return _fallback;
    }

    if (const long* whole = std::get_if<long>(found))
    {
        return static_cast<double>(*whole);
    }

    if (const double* real = std::get_if<double>(found))
    {
        return *real;
    }

    return _fallback;
}

// visible_order
//   the rows the grid and the tree show, in the order they show them.
std::vector<int>
visible_order(
    const model& _model
)
{
    std::vector<int> out;

    for (const resolved_row* each : shown_rows(_model))
    {
        if (each->visible)
        {
            out.push_back(each->id);
        }
    }

    return out;
}

// byte_bits
//   the bits one byte covers, by the sheet's byte numbering.
void
byte_bits(
    const sheet& _sheet,
    int          _byte,
    int&         _low,
    int&         _high
)
{
    if (_sheet.bytes == byte_order::little)
    {
        _low = 8 * _byte;
    }
    else
    {
        _low = _sheet.width - 8 * (_byte + 1);
    }

    _high = _low + 7;
    _low  = std::clamp(_low, 0, _sheet.width - 1);
    _high = std::clamp(_high, 0, _sheet.width - 1);

    return;
}

// sheet_number
//   a number typed for a sheet -- an operand, the probe -- as the sheet's
// bits: a negative one is its two's complement in the sheet's width.
bool
sheet_number(
    const std::string& _text,
    int                _width,
    wide&              _bits
)
{
    const parsed_wide parsed = parse_wide(_text);

    if (!parsed.ok)
    {
        return false;
    }

    _bits = ( (parsed.negative) &&
              (is_wide(_width)) )
                ? widen_signed(static_cast<i64>(parsed.value.low()), _width)
                : parsed.value;

    return true;
}

// remembered
//   struct: what the settings file holds of the window -- its theme and
// colour scheme, and the user's preferences -- apart from the window, which
// ImGui's context may outlive. The layout of the windows is ImGui's own,
// in the same file.
struct remembered
{
    int         theme  = 0;
    int         scheme = 0;
    preferences prefs;
    bool        read   = false;   // the file said something of it
};

remembered&
memory()
{
    static remembered held;

    return held;
}

// ini_open / ini_line / ini_write
//   the settings handler's three parts: the section it owns, one of its
// lines read, and all of it written.
void*
ini_open(
    ImGuiContext*         _context,
    ImGuiSettingsHandler* _handler,
    const char*           _name
)
{
    (void)_context;
    (void)_handler;

    return (std::strcmp(_name, "window") == 0) ? &memory() : nullptr;
}

void
ini_line(
    ImGuiContext*         _context,
    ImGuiSettingsHandler* _handler,
    void*                 _entry,
    const char*           _line
)
{
    (void)_context;
    (void)_handler;
    (void)_entry;

    remembered&       held = memory();
    const char* const at   = std::strchr(_line, '=');

    if (at == nullptr)
    {
        return;
    }

    const std::string key(_line, static_cast<std::size_t>(at - _line));
    const std::string value(at + 1);
    const long        number = std::strtol(value.c_str(), nullptr, 0);

    // within
    //   a small whole number held to [0, _count), or _field as it was.
    auto within = [number](int& _field,
                           int  _count)
    {
        if ( (number >= 0) &&
             (number < _count) )
        {
            _field = static_cast<int>(number);
        }

        return;
    };

    held.read = true;

    if (key == "theme")
    {
        within(held.theme, THEME_COUNT);
    }
    else if (key == "scheme")
    {
        within(held.scheme, SCHEME_COUNT);
    }
    else if (key == "merged")
    {
        held.prefs.merged = (number != 0);
    }
    else if (key == "add")
    {
        int style = static_cast<int>(held.prefs.add);

        within(style, 2);
        held.prefs.add = static_cast<add_style>(style);
    }
    else if (key == "add_kind")
    {
        within(held.prefs.add_kind, ADD_KINDS);
    }
    else if (key == "new_colour")
    {
        int mode = static_cast<int>(held.prefs.colour);

        within(mode, 3);
        held.prefs.colour = static_cast<new_colour>(mode);
    }
    else if (key == "new_rgba")
    {
        held.prefs.colour_rgba =
            static_cast<rgba>(std::strtoul(value.c_str(), nullptr, 16));
    }
    else if (key == "restore")
    {
        held.prefs.restore = (number != 0);
    }
    else if ( (key == "settings_file") &&
              (!value.empty()) )
    {
        held.prefs.settings_files.push_back(value);
    }

    return;
}

void
ini_write(
    ImGuiContext*         _context,
    ImGuiSettingsHandler* _handler,
    ImGuiTextBuffer*      _out
)
{
    (void)_context;

    const remembered& held = memory();

    _out->appendf("[%s][window]\n", _handler->TypeName);
    _out->appendf("theme=%d\nscheme=%d\nmerged=%d\nadd=%d\nadd_kind=%d\n"
                  "new_colour=%d\nnew_rgba=%08X\nrestore=%d\n",
                  held.theme,
                  held.scheme,
                  held.prefs.merged ? 1 : 0,
                  static_cast<int>(held.prefs.add),
                  held.prefs.add_kind,
                  static_cast<int>(held.prefs.colour),
                  static_cast<unsigned int>(held.prefs.colour_rgba),
                  held.prefs.restore ? 1 : 0);

    for (const std::string& each : held.prefs.settings_files)
    {
        _out->appendf("settings_file=%s\n", each.c_str());
    }

    _out->append("\n");

    return;
}

// remember
//   the theme and scheme and the preferences, to be written to the
// settings file.
void
remember(
    const shell_state& _shell
)
{
    memory().theme  = _shell.theme;
    memory().scheme = _shell.scheme;
    memory().prefs  = _shell.prefs;
    memory().read   = true;

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::MarkIniSettingsDirty();
    }

    return;
}

// file_parts
//   a path's file name, the name without its extension, and the
// extension, lower-cased, without its dot.
void
file_parts(
    const std::string& _path,
    std::string&       _name,
    std::string&       _stem,
    std::string&       _extension
)
{
    const std::size_t slash = _path.find_last_of("/\\");

    _name = (slash == std::string::npos) ? _path : _path.substr(slash + 1);

    const std::size_t dot = _name.rfind('.');

    _stem      = (dot == std::string::npos) ? _name : _name.substr(0, dot);
    _extension = (dot == std::string::npos) ? std::string()
                                            : _name.substr(dot + 1);

    for (char& each : _extension)
    {
        each = static_cast<char>(
            std::tolower(static_cast<unsigned char>(each)));
    }

    return;
}

/*
json_stem
  A JSON file's name with what says its shape taken off: uart_cr for
uart_cr.bitwisedom.json, uart_cr.bitfield.json or uart_cr.export.json.
*/
std::string
json_stem(
    const std::string& _path
)
{
    std::string name;
    std::string stem;
    std::string extension;

    file_parts(_path, name, stem, extension);

    for (const char* shape : { ".bitwisedom", ".bitfield", ".export" })
    {
        const std::string tail = shape;

        if ( (stem.size() > tail.size()) &&
             (stem.compare(stem.size() - tail.size(), tail.size(), tail) ==
              0) )
        {
            stem.erase(stem.size() - tail.size());

            break;
        }
    }

    return stem;
}

/*
readable
  A settings file's text without the lines ImGui would read back wrongly: a
number that is no number -- the weight of a table's column, saved before
the table had laid itself out, is "nan" -- would leave the column no width
at all. The column, without its line, takes its default.
*/
std::string
readable(
    const std::string& _text
)
{
    std::string out;
    std::size_t at = 0;

    while (at < _text.size())
    {
        std::size_t end = _text.find('\n', at);

        if (end == std::string::npos)
        {
            end = _text.size();
        }

        const std::string line   = _text.substr(at, end - at);
        const std::size_t equals = line.find('=');
        bool              keep   = true;

        if (equals != std::string::npos)
        {
            const char*  value  = line.c_str() + equals + 1;
            char*        stop   = nullptr;
            const double number = std::strtod(value, &stop);

            keep = ( (stop == value) ||
                     (std::isfinite(number)) );
        }

        if (keep)
        {
            out += line;
            out += '\n';
        }

        at = end + 1;
    }

    return out;
}

/*
utf8_path
  A file's name as the system means it: names travel through the window as
UTF-8, and a path made from UTF-8 says so, on every platform.
*/
std::filesystem::path
utf8_path(
    const std::string& _utf8
)
{
#if defined(__cpp_char8_t)
    return std::filesystem::path(std::u8string(_utf8.begin(), _utf8.end()));
#else
    return std::filesystem::u8path(_utf8);
#endif
}

/*
new_row
  A row as the user adds it: in the colour the preferences give new rows --
none of its own, its bits drawn plain; the palette's, each its own, as a
row left to the palette is; or the one colour, every new row alike.
*/
row
new_row(
    const preferences& _prefs
)
{
    row made;

    switch (_prefs.colour)
    {
        case new_colour::none:
            made.transparent = true;
            break;

        case new_colour::automatic:
            break;

        case new_colour::fixed:
            made.colour = _prefs.colour_rgba;
            break;
    }

    return made;
}

// drawing
//   the window the renderers draw for: the one whose frame registered them
// last. A second window -- a test's, in an ImGui of its own -- registers
// its own, and each frame puts back those of the window it draws.
const void*&
drawing()
{
    static const void* held = nullptr;

    return held;
}

// SESSION_FORMAT / SESSION_VERSION / SESSION_PAUSE
//   constants: what a session file calls itself; the newest version this
// build reads; and how long, in seconds, the window waits after a change
// before it writes the session again -- so a drag across the grid writes
// it once, not every frame.
const char* const SESSION_FORMAT  = "bitwisedom.session";
const int         SESSION_VERSION = 1;
const double      SESSION_PAUSE   = 2.0;

// VALUE_FLOOR
//   constant: the narrowest the grid's value column is dragged or fitted
// to. The settings panel may hide it, at 0; a drag may not, since a hidden
// column has no edge to drag it back by.
const float VALUE_FLOOR = 32.0f;

// within_setting
//   a width held to the bounds its setting has, so a drag in the grid and
// the settings panel agree on how wide a column may be.
float
within_setting(
    const char* _path,
    float       _width
)
{
    const setting* found = find_setting(_path);

    if (found == nullptr)
    {
        return _width;
    }

    return std::clamp(_width,
                      static_cast<float>(found->low),
                      static_cast<float>(found->high));
}

}  // anonymous namespace


namespace
{

/*
config_file
  A file in the directory the window keeps what it remembers in, made if
it is not there: %APPDATA%\bitwisedom on Windows; elsewhere
$XDG_CONFIG_HOME/bitwisedom, or ~/.config/bitwisedom. The path is UTF-8,
which is how ImGui and read_file open a file by name; empty when there is
nowhere to keep one.
*/
std::string
config_file(
    const char* _name
)
{
    namespace fs = std::filesystem;

    fs::path where;

#if defined(_WIN32)
    // wide, so a user name outside the code page still names the folder
    const wchar_t* appdata = _wgetenv(L"APPDATA");

    if ( (appdata == nullptr) ||
         (appdata[0] == L'\0') )
    {
        return std::string();
    }

    where = fs::path(appdata) / "bitwisedom";
#else
    const char* config = std::getenv("XDG_CONFIG_HOME");
    const char* home   = std::getenv("HOME");

    if ( (config != nullptr) &&
         (config[0] != '\0') )
    {
        where = fs::path(config) / "bitwisedom";
    }
    else if ( (home != nullptr) &&
              (home[0] != '\0') )
    {
        where = fs::path(home) / ".config" / "bitwisedom";
    }
    else
    {
        return std::string();
    }
#endif

    std::error_code failed;

    fs::create_directories(where, failed);

    if (failed)
    {
        return std::string();
    }

    const auto named = (where / _name).u8string();

    return std::string(named.begin(), named.end());
}

}  // anonymous namespace


std::string
preferences_path()
{
    return config_file("bitwisedom.ini");
}

std::string
session_path()
{
    return config_file("session.json");
}


// --------------------------------------------------------------- utilities

std::string
unique_name(
    const sheet&       _sheet,
    const std::string& _base
)
{
    std::vector<std::string> taken;

    walk(_sheet,
         [&taken](const row& _row, const row&, int)
         {
             taken.push_back(_row.name);
         });

    std::string name  = _base;
    int         count = 1;

    while (std::find(taken.begin(), taken.end(), name) != taken.end())
    {
        name = _base + "_" + format_dec(static_cast<u64>(++count));
    }

    return name;
}

std::vector<int>
range_choices(
    const sheet& _sheet
)
{
    std::vector<int> ids;

    walk(_sheet,
         [&ids](const row& _row, const row&, int)
         {
             if ( (_row.kind == row_kind::symbol) &&
                  (_row.def == def_kind::range) )
             {
                 ids.push_back(_row.id);
             }
         });

    return ids;
}

std::vector<int>
container_choices(
    const sheet& _sheet
)
{
    std::vector<int> ids = { 0 };

    walk(_sheet,
         [&ids](const row& _row, const row&, int)
         {
             if (can_contain(_row))
             {
                 ids.push_back(_row.id);
             }
         });

    return ids;
}

const char* const NODE_KINDS[NODE_KIND_COUNT] = {
    "Bit", "Range", "Value in a range", "Literal", "Expression", "Macro",
    "Group" };

std::vector<int>
op_targets(
    const model& _model
)
{
    return _model.ui.selection;
}


// ------------------------------------------------------------------- the app

app::app(
    std::string _preferences,
    std::string _session
)
    : m_preferences(std::move(_preferences)),
      m_session(std::move(_session))
{
    // windows dock as Visual Studio's do: dragged by their tabs onto
    // another window's, or to an edge of one
    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    // the window's own section of ImGui's settings file, beside the
    // layout of its windows and the tables' columns
    if ( (ImGui::GetCurrentContext() != nullptr) &&
         (ImGui::FindSettingsHandler("bitwisedom") == nullptr) )
    {
        ImGuiSettingsHandler handler;
        handler.TypeName   = "bitwisedom";
        handler.TypeHash   = ImHashStr("bitwisedom");
        handler.ReadOpenFn = ini_open;
        handler.ReadLineFn = ini_line;
        handler.WriteAllFn = ini_write;
        ImGui::AddSettingsHandler(&handler);
    }

    // the settings file, read before the first frame: ImGui's windows and
    // tables, and the window's own through the handler above
    std::string text;
    std::string error;

    if ( (!m_preferences.empty())                  &&
         (ImGui::GetCurrentContext() != nullptr)   &&
         (read_file(m_preferences, text, error)) )
    {
        const std::string kept = readable(text);

        ImGui::LoadIniSettingsFromMemory(kept.c_str(), kept.size());
    }

    // what the settings file remembered of the window: its theme, its
    // scheme and the user's preferences -- the settings files on the list
    // only while they are where they were
    if (memory().read)
    {
        namespace fs = std::filesystem;

        m_shell.theme  = memory().theme;
        m_shell.scheme = memory().scheme;
        m_shell.prefs  = memory().prefs;

        std::vector<std::string>& files = m_shell.prefs.settings_files;
        std::error_code           failed;

        files.erase(std::remove_if(files.begin(),
                                   files.end(),
                                   [&failed](const std::string& _path)
                                   {
                                       return !fs::exists(utf8_path(_path),
                                                          failed);
                                   }),
                    files.end());
        remember(m_shell);
    }

    register_renderers([this](int _id) -> model*
                       {
                           return (_id == 0) ? m_doc : find_document(_id);
                       },
                       [this]() -> const std::vector<action_entry>*
                       {
                           return &m_shell.actions;
                       });
    drawing() = this;

    // where it was left -- or, the first time, an example to look at
    if (!restore_session())
    {
        (void)add_workspace();
        (void)open_document();
        load_sheet_into(make_example("uart_cr"), "uart_cr.bitwisedom.json");
        doc().ui.browsing = true;
        status("uart_cr: an example -- File > Examples has more", false);
    }
}

app::~app()
{
    keep_preferences();
    keep_session(true);

    if (drawing() == this)
    {
        drawing() = nullptr;
    }

    return;
}

/*
keep_preferences
  The settings file, written whenever ImGui says its settings have changed,
and when the window closes: what ImGui holds, without what it could not
read back.
*/
void
app::keep_preferences()
{
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return;
    }

    ImGui::GetIO().WantSaveIniSettings = false;

    if (m_preferences.empty())
    {
        return;
    }

    std::size_t       size  = 0;
    const char* const saved = ImGui::SaveIniSettingsToMemory(&size);
    std::string       error;

    (void)write_file(m_preferences,
                     readable(std::string(saved, size)),
                     error);

    return;
}

/*
keep_session
  The session file: every workspace, every document in them -- its sheet
with its unsaved changes, where it is kept, what is selected in it, its
operation -- and each window's zoom. It is written a moment after what it
keeps last changed, and at once when the window closes; the layout of the
windows is ImGui's, in the settings file.
*/
void
app::keep_session(
    bool _now
)
{
    if ( (m_session.empty()) ||
         (m_doc == nullptr) )
    {
        return;
    }

    // a mark that moves whenever anything kept changes
    long mark = static_cast<long>(m_shell.version) * 1000003L;

    for (const std::unique_ptr<model>& each : m_docs)
    {
        mark += static_cast<long>(each->version);
    }

    const double time = (ImGui::GetCurrentContext() != nullptr)
                            ? ImGui::GetTime()
                            : 0.0;

    if ( (!_now) &&
         ( (mark == m_session_mark) ||
           (time - m_session_time < SESSION_PAUSE) ) )
    {
        return;
    }

    json_value root   = json_value::object();
    json_value zooms  = json_value::object();
    json_value spaces = json_value::array();
    json_value sheets = json_value::array();

    // numbers
    //   ids as a JSON array.
    auto numbers = [](const std::vector<int>& _ids)
    {
        json_value out = json_value::array();

        for (int each : _ids)
        {
            out.push(json_value::number(each));
        }

        return out;
    };

    root.set("format", json_value::string(SESSION_FORMAT));
    root.set("version", json_value::number(SESSION_VERSION));
    root.set("current", json_value::number(
                            static_cast<i64>(m_shell.current)));
    root.set("next_document", json_value::number(m_next));
    root.set("next_workspace", json_value::number(m_next_ws));
    root.set("focused", json_value::string(m_shell.focused));

    for (const auto& each : m_shell.zoom)
    {
        zooms.set(each.first, json_value::real(each.second));
    }

    root.set("zoom", std::move(zooms));

    for (const workspace& each : m_shell.workspaces)
    {
        json_value one    = json_value::object();
        json_value tools  = json_value::array();
        json_value stowed = json_value::array();

        for (int i = 0; i < TOOL_COUNT; ++i)
        {
            if (each.tools[static_cast<std::size_t>(i)])
            {
                tools.push(json_value::string(
                    tool_key(static_cast<tool>(i))));
            }
        }

        for (int i : each.stowed)
        {
            stowed.push(json_value::string(tool_key(static_cast<tool>(i))));
        }

        one.set("id", json_value::number(each.id));
        one.set("name", json_value::string(each.name));
        one.set("documents", numbers(each.documents));
        one.set("active", json_value::number(each.active));
        one.set("tools", std::move(tools));
        one.set("stowed", std::move(stowed));
        spaces.push(std::move(one));
    }

    root.set("workspaces", std::move(spaces));

    for (const std::unique_ptr<model>& each : m_docs)
    {
        const ui_state& ui  = each->ui;
        json_value      one = json_value::object();

        one.set("id", json_value::number(each->id));
        one.set("path", json_value::string(ui.path));
        one.set("file", json_value::string(ui.file));
        one.set("dirty", json_value::boolean(ui.dirty));
        one.set("pristine", json_value::boolean(ui.pristine));
        one.set("browsing", json_value::boolean(ui.browsing));
        one.set("selection", numbers(ui.selection));
        one.set("anchor", json_value::number(ui.anchor));
        one.set("op", json_value::number(ui.op));
        one.set("operand", json_value::string(ui.operand));
        one.set("op_probe", json_value::boolean(ui.op_probe));
        one.set("op_preview", json_value::boolean(ui.op_preview));
        one.set("sheet", sheet_object(each->doc));
        sheets.push(std::move(one));
    }

    root.set("documents", std::move(sheets));

    std::string error;

    (void)write_file(m_session, write_json(root) + "\n", error);

    m_session_mark = mark;
    m_session_time = time;

    return;
}

/*
restore_session
  The window as the session file left it, when the user would have it open
where it was left: its workspaces, its documents -- a sheet saved and
unchanged read again from its file, which may have changed since -- the
one in front of each, their tool windows and each window's zoom. False,
and nothing changed, when there is no session to open.
*/
bool
app::restore_session()
{
    std::string text;
    std::string error;
    json_value  root;

    if ( (m_session.empty())                                      ||
         (!m_shell.prefs.restore)                                 ||
         (!read_file(m_session, text, error))                     ||
         (!parse_json(text, root, error))                         ||
         (root.type != json_type::object)                         ||
         (root.find("format") == nullptr)                         ||
         (root.find("format")->as_string("") != SESSION_FORMAT) )
    {
        return false;
    }

    const json_value* sheets = root.find("documents");
    const json_value* spaces = root.find("workspaces");

    if ( (sheets == nullptr)                  ||
         (spaces == nullptr)                  ||
         (sheets->type != json_type::array)   ||
         (spaces->type != json_type::array) )
    {
        return false;
    }

    // whole
    //   a member's whole number, or _fallback.
    auto whole = [](const json_value& _object,
                    const char*       _key,
                    i64               _fallback)
    {
        const json_value* found = _object.find(_key);

        return (found != nullptr) ? found->as_int(_fallback) : _fallback;
    };

    // truth / words
    //   a member's flag, or _fallback; a member's text, or _fallback.
    auto truth = [](const json_value& _object,
                    const char*       _key,
                    bool              _fallback)
    {
        const json_value* found = _object.find(_key);

        return (found != nullptr) ? found->as_bool(_fallback) : _fallback;
    };
    auto words = [](const json_value&  _object,
                    const char*        _key,
                    const std::string& _fallback)
    {
        const json_value* found = _object.find(_key);

        return (found != nullptr) ? found->as_string(_fallback) : _fallback;
    };

    // ids
    //   a member's array of whole numbers.
    auto ids = [](const json_value& _object,
                  const char*       _key)
    {
        std::vector<int>  out;
        const json_value* found = _object.find(_key);

        if ( (found != nullptr) &&
             (found->type == json_type::array) )
        {
            for (const json_value& each : found->items)
            {
                out.push_back(static_cast<int>(each.as_int(0)));
            }
        }

        return out;
    };

    std::vector<std::unique_ptr<model>> made;

    for (const json_value& each : sheets->items)
    {
        std::unique_ptr<model> one  = std::make_unique<model>();
        const json_value*      kept = each.find("sheet");
        ui_state&              ui   = one->ui;

        one->id    = static_cast<int>(whole(each, "id", 0));
        one->today = today();

        const bool taken =
            std::any_of(made.begin(),
                        made.end(),
                        [&one](const std::unique_ptr<model>& _other)
                        {
                            return (_other->id == one->id);
                        });

        if ( (one->id <= 0)     ||
             (taken)            ||
             (kept == nullptr)  ||
             (!sheet_from_object(*kept, one->doc, error)) )
        {
            continue;
        }

        ui.path       = words(each, "path", ui.path);
        ui.file       = words(each, "file", ui.path);
        ui.dirty      = truth(each, "dirty", false);
        ui.pristine   = truth(each, "pristine", false);
        ui.browsing   = truth(each, "browsing", false);
        ui.selection  = ids(each, "selection");
        ui.anchor     = static_cast<int>(whole(each, "anchor", 0));
        ui.op         = static_cast<int>(std::clamp<i64>(
                            whole(each, "op", 0), 0, OP_COUNT - 1));
        ui.operand    = words(each, "operand", ui.operand);
        ui.op_probe   = truth(each, "op_probe", false);
        ui.op_preview = truth(each, "op_preview", true);

        // a sheet saved, and unchanged since, is its file's -- which may
        // have changed while the window was closed
        if ( (!ui.dirty)         &&
             (!ui.browsing)      &&
             (!ui.file.empty()) )
        {
            std::string body;
            std::string failed;
            sheet       read;

            if ( (read_file(ui.file, body, failed)) &&
                 (load_sheet(body, read, failed)) )
            {
                one->doc = std::move(read);
            }
        }

        made.push_back(std::move(one));
    }

    std::vector<workspace> laid;
    std::vector<int>       claimed;

    for (const json_value& each : spaces->items)
    {
        workspace one;

        one.id   = static_cast<int>(whole(each, "id", 0));
        one.name = words(each, "name", std::string());

        const bool taken = std::any_of(laid.begin(),
                                       laid.end(),
                                       [&one](const workspace& _other)
                                       {
                                           return (_other.id == one.id);
                                       });

        if ( (one.id <= 0) ||
             (taken) )
        {
            continue;
        }

        if (one.name.empty())
        {
            one.name = "Workspace " + format_dec(static_cast<u64>(one.id));
        }

        // its documents: each one that was read, and is no other's
        for (int id : ids(each, "documents"))
        {
            const bool read =
                std::any_of(made.begin(),
                            made.end(),
                            [id](const std::unique_ptr<model>& _doc)
                            {
                                return (_doc->id == id);
                            });

            if ( (read) &&
                 (std::find(claimed.begin(), claimed.end(), id) ==
                  claimed.end()) )
            {
                one.documents.push_back(id);
                claimed.push_back(id);
            }
        }

        one.active = static_cast<int>(whole(each, "active", 0));

        if (std::find(one.documents.begin(), one.documents.end(),
                      one.active) == one.documents.end())
        {
            one.active = one.documents.empty() ? 0 : one.documents.back();
        }

        // tools
        //   a member's tool windows, by their keys.
        auto tools = [&each](const char* _key)
        {
            std::vector<int>  out;
            const json_value* found = each.find(_key);

            if ( (found != nullptr) &&
                 (found->type == json_type::array) )
            {
                for (const json_value& name : found->items)
                {
                    for (int i = 0; i < TOOL_COUNT; ++i)
                    {
                        if (name.as_string("") ==
                            tool_key(static_cast<tool>(i)))
                        {
                            out.push_back(i);
                        }
                    }
                }
            }

            return out;
        };

        for (int i : tools("tools"))
        {
            one.tools[static_cast<std::size_t>(i)] = true;
        }

        one.stowed = tools("stowed");
        laid.push_back(std::move(one));
    }

    if ( (made.empty()) ||
         (laid.empty()) )
    {
        return false;
    }

    // a document no workspace claims goes to the first
    for (const std::unique_ptr<model>& each : made)
    {
        if (std::find(claimed.begin(), claimed.end(), each->id) ==
            claimed.end())
        {
            laid.front().documents.push_back(each->id);
        }
    }

    int most_doc = 0;
    int most_ws  = 0;

    for (const std::unique_ptr<model>& each : made)
    {
        most_doc = std::max(most_doc, each->id);
    }

    for (const workspace& each : laid)
    {
        most_ws = std::max(most_ws, each.id);
    }

    m_docs             = std::move(made);
    m_shell.workspaces = std::move(laid);
    m_shell.current    = static_cast<std::size_t>(std::clamp<i64>(
        whole(root, "current", 0),
        0,
        static_cast<i64>(m_shell.workspaces.size()) - 1));
    m_next             = std::max(static_cast<int>(
                                      whole(root, "next_document", 0)),
                                  most_doc + 1);
    m_next_ws          = std::max(static_cast<int>(
                                      whole(root, "next_workspace", 0)),
                                  most_ws + 1);
    m_shell.focused    = words(root, "focused", std::string());

    const json_value* zooms = root.find("zoom");

    if ( (zooms != nullptr) &&
         (zooms->type == json_type::object) )
    {
        for (const json_member& each : zooms->members)
        {
            const float level = static_cast<float>(each.value.as_real(1.0));

            if ( (level >= ZOOM_LEAST) &&
                 (level <= ZOOM_MOST) )
            {
                m_shell.zoom[each.key] = level;
            }
        }
    }

    // the document in front: the front workspace's -- a new one, for a
    // workspace left empty
    const int front = current().active;

    m_doc = (front != 0) ? find_document(front) : nullptr;

    if (m_doc == nullptr)
    {
        m_doc = m_docs.front().get();
        to_workspace(m_shell.current);
    }

    // its window in front of its dock
    current().raise = m_doc->id;

    restyle();
    shell_changed();
    status("opened where it was left: " +
               format_dec(m_docs.size()) + " sheet(s) in " +
               format_dec(m_shell.workspaces.size()) + " workspace(s)",
           false);

    // what was read is what the file holds
    m_session_mark = -1;

    return true;
}

void
app::load_fonts(
    float _size
)
{
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromMemoryCompressedTTF(fonts::interface_data(),
                                             fonts::interface_size(),
                                             _size);
    mono_font() = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::mono_data(),
                                                           fonts::mono_size(),
                                                           _size);

    return;
}

bool
app::quitting() const
{
    return m_shell.quit;
}

const model&
app::state() const
{
    return *m_doc;
}

const shell_state&
app::shell() const
{
    return m_shell;
}

std::vector<const model*>
app::documents() const
{
    std::vector<const model*> out;

    for (const std::unique_ptr<model>& each : m_docs)
    {
        out.push_back(each.get());
    }

    return out;
}

const model*
app::document(
    int _id
) const
{
    for (const std::unique_ptr<model>& each : m_docs)
    {
        if (each->id == _id)
        {
            return each.get();
        }
    }

    return nullptr;
}

void
app::post(
    const event& _event
)
{
    m_queue.push_back(_event);

    return;
}

// ------------------------------------------------- documents and workspaces

model&
app::doc()
{
    return *m_doc;
}

const model&
app::doc() const
{
    return *m_doc;
}

model*
app::find_document(
    int _id
)
{
    for (const std::unique_ptr<model>& each : m_docs)
    {
        if (each->id == _id)
        {
            return each.get();
        }
    }

    return nullptr;
}

/*
open_document
  A new, empty document in the workspace in front, put in front of it: the
look the window has, and today's date for its header's banner.
*/
model&
app::open_document()
{
    std::unique_ptr<model> made = std::make_unique<model>();

    made->id        = m_next++;
    made->today     = today();
    made->ui.theme  = m_shell.theme;
    made->ui.scheme = m_shell.scheme;
    made->shown     = make_look(made->doc, m_shell.theme, m_shell.scheme);
    made->res       = resolve(made->doc, made->shown.colours);
    sort_rows(*made);

    model& out = *made;

    m_docs.push_back(std::move(made));
    current().documents.push_back(out.id);
    activate(out.id);

    return out;
}

/*
activate
  A document put in front: of its workspace, which is put in front of the
window if it is not already -- and its window brought to the front of its
dock, unless the user brought it there already, by clicking in it. A
document already in front stays so, and nothing is built again.
*/
void
app::activate(
    int  _id,
    bool _raise
)
{
    model* found = find_document(_id);

    if (found == nullptr)
    {
        return;
    }

    workspace* holder = holding(_id);

    if ( (found == m_doc)                       &&
         (holder == &current())                 &&
         (current().active == _id) )
    {
        return;
    }

    m_doc = found;

    if (holder != nullptr)
    {
        holder->active  = _id;
        holder->raise   = _raise ? _id : 0;
        m_shell.current = static_cast<std::size_t>(
                              holder - m_shell.workspaces.data());
    }

    shell_changed();

    return;
}

workspace&
app::current()
{
    return m_shell.workspaces[m_shell.current];
}

workspace*
app::holding(
    int _document
)
{
    for (workspace& each : m_shell.workspaces)
    {
        if (std::find(each.documents.begin(), each.documents.end(),
                      _document) != each.documents.end())
        {
            return &each;
        }
    }

    return nullptr;
}

/*
add_workspace
  A new workspace, with the tool windows a workspace opens with -- the
symbols tree among them only when the user keeps it apart from the grid --
and no documents yet. It is added, not put in front.
*/
workspace&
app::add_workspace()
{
    workspace made;

    made.id   = m_next_ws++;
    made.name = "Workspace " + format_dec(static_cast<u64>(made.id));

    for (int i = 0; i < TOOL_COUNT; ++i)
    {
        const tool each = static_cast<tool>(i);

        made.tools[static_cast<std::size_t>(i)] =
            (each == tool::symbols) ? (!m_shell.prefs.merged)
                                    : tool_open(each);
    }

    m_shell.workspaces.push_back(made);
    shell_changed();

    return m_shell.workspaces.back();
}

/*
to_workspace
  A workspace put in front, with the document in front of it -- a new one,
if it has none.
*/
void
app::to_workspace(
    std::size_t _index
)
{
    if (_index >= m_shell.workspaces.size())
    {
        return;
    }

    m_shell.current = _index;

    if (current().documents.empty())
    {
        (void)open_document();
        load_sheet_into(make_document("untitled", 32),
                        "untitled.bitwisedom.json");
        doc().ui.pristine = true;
    }
    else
    {
        const int front = current().active;

        activate( (find_document(front) != nullptr)
                      ? front
                      : current().documents.back() );
    }

    status(current().name, false);

    return;
}

/*
move_document
  A document taken from its workspace to another, docked there among the
other documents, and put in front of it -- and that workspace in front of
the window. The workspace it left keeps a document: a new one, if it was
its last.
*/
void
app::move_document(
    int         _document,
    std::size_t _index
)
{
    workspace* from = holding(_document);

    if ( (from == nullptr) ||
         (_index >= m_shell.workspaces.size()) ||
         (&m_shell.workspaces[_index] == from) )
    {
        return;
    }

    std::vector<int>& left = from->documents;

    left.erase(std::remove(left.begin(), left.end(), _document), left.end());

    if (from->active == _document)
    {
        from->active = left.empty() ? 0 : left.back();
    }

    workspace& to = m_shell.workspaces[_index];

    to.documents.push_back(_document);
    to.placed.push_back(_document);
    activate(_document);
    status("moved to " + to.name, false);

    return;
}

/*
close_document
  A document closed, unless it has changes and the user has not said to
lose them: then the dialog asks first. The workspace it was in puts
another document in front -- a new one, if it was the last.
*/
void
app::close_document(
    int  _id,
    bool _discard
)
{
    model* closing = find_document(_id);

    if (closing == nullptr)
    {
        return;
    }

    if ( (closing->ui.dirty) &&
         (!_discard) )
    {
        m_closing      = _id;
        m_closing_open = true;
        activate(_id);

        return;
    }

    workspace* from = holding(_id);

    if (from != nullptr)
    {
        std::vector<int>& left = from->documents;

        left.erase(std::remove(left.begin(), left.end(), _id), left.end());

        if (from->active == _id)
        {
            from->active = left.empty() ? 0 : left.back();
        }
    }

    const bool        front = (m_doc == closing);
    const std::string name  = closing->ui.path;

    // its window is forgotten with it: a document opened later never
    // takes its place
    m_forget.push_back("###" + document_key(_id));

    for (std::size_t i = 0; i < closing->doc.definitions.size(); ++i)
    {
        m_forget.push_back("###" + document_key(_id) + ".def" +
                           format_dec(static_cast<u64>(i)));
    }

    m_docs.erase(std::remove_if(m_docs.begin(),
                                m_docs.end(),
                                [_id](const std::unique_ptr<model>& _each)
                                {
                                    return (_each->id == _id);
                                }),
                 m_docs.end());

    if (front)
    {
        m_doc = nullptr;

        const std::size_t index =
            (from == nullptr) ? m_shell.current
                              : static_cast<std::size_t>(
                                    from - m_shell.workspaces.data());

        m_shell.current = index;

        if (current().documents.empty())
        {
            (void)open_document();
            load_sheet_into(make_document("untitled", 32),
                            "untitled.bitwisedom.json");
            doc().ui.pristine = true;
        }
        else
        {
            activate(current().active);
        }
    }

    status("closed " + name, false);
    shell_changed();

    return;
}

/*
tool_to
  A tool window of the workspace in front opened or closed, and brought to
the front of its dock once.
*/
void
app::tool_to(
    tool _tool,
    bool _open,
    bool _focus
)
{
    const std::size_t index = static_cast<std::size_t>(_tool);

    current().tools[index] = _open;
    current().focus[index] = ( (_open) &&
                               (_focus) );
    shell_changed();

    return;
}

void
app::shell_changed()
{
    ++m_shell.version;

    return;
}

/*
zoom
  A window's zoom, by its key, a step in or out -- or, for a step of 0,
back to 1.
*/
void
app::zoom(
    const std::string& _key,
    float              _step
)
{
    if (_key.empty())
    {
        return;
    }

    if (_step == 0.0f)
    {
        m_shell.zoom.erase(_key);
    }
    else
    {
        const float now  = zoom_of(m_shell, _key);
        const float next = std::clamp(
            std::round((now + _step) * 100.0f) / 100.0f,
            ZOOM_LEAST,
            ZOOM_MOST);

        if (std::fabs(next - 1.0f) < 0.001f)
        {
            m_shell.zoom.erase(_key);
        }
        else
        {
            m_shell.zoom[_key] = next;
        }
    }

    status("zoom " + format_dec(static_cast<u64>(
                         std::lround(zoom_of(m_shell, _key) * 100.0f))) + "%",
           false);
    shell_changed();

    return;
}

void
app::status(
    const std::string& _text,
    bool               _error
)
{
    doc().ui.status       = _text;
    doc().ui.status_error = _error;
    ++doc().version;

    return;
}

void
app::changed()
{
    doc().ui.theme        = m_shell.theme;
    doc().ui.scheme       = m_shell.scheme;
    doc().shown           = make_look(doc().doc,
                                      m_shell.theme,
                                      m_shell.scheme);
    doc().res             = resolve(doc().doc, doc().shown.colours);
    sort_rows(doc());
    doc().preview_valid   = false;
    doc().header_valid    = false;
    ++doc().version;

    // the Add node window's row, tried on the sheet as it is now
    if (m_shell.node_open)
    {
        refresh_node();
    }

    // a selected row that no longer exists is no longer selected
    std::vector<int>& selection = doc().ui.selection;

    selection.erase(std::remove_if(selection.begin(),
                                   selection.end(),
                                   [this](int _id)
                                   {
                                       return (doc().res.find(_id) ==
                                               nullptr);
                                   }),
                    selection.end());

    return;
}

template<typename Change>
void
app::edit(
    const std::string& _key,
    Change             _change
)
{
    const bool was = doc().ui.dirty;

    doc().steps.record(doc().doc, _key);
    _change(doc().doc);
    doc().ui.dirty    = true;
    doc().ui.pristine = false;
    doc().ui.browsing = false;
    changed();

    // its title says so
    if (!was)
    {
        shell_changed();
    }

    return;
}

template<typename Change>
void
app::quietly(
    Change _change
)
{
    const bool was = doc().ui.dirty;

    _change(doc().doc);
    doc().ui.dirty    = true;
    doc().ui.pristine = false;
    doc().ui.browsing = false;
    changed();

    if (!was)
    {
        shell_changed();
    }

    return;
}

/*
open_all
  A group and every group within it opened or closed -- every group the
sheet has, for the root's id. As one group's arrow does, it changes how the
sheet is seen, which is saved with it, and is not a step to undo. A sheet
already so is left as it was, and not marked changed.
*/
void
app::open_all(
    int  _group,
    bool _open
)
{
    const int count = open_groups(doc().doc, _group, _open);

    if (count > 0)
    {
        if (!doc().ui.dirty)
        {
            shell_changed();
        }

        doc().ui.dirty    = true;
        doc().ui.pristine = false;
        doc().ui.browsing = false;
        changed();
    }

    // what was asked for: every group, or one and the groups within it
    const row*        group = find_row(doc().doc, _group);
    const bool        every = (_group == doc().doc.root.id);
    const std::string state = _open ? "open" : "closed";
    const std::string what  =
        every ? std::string("every group")
              : ( ( (group != nullptr) &&
                    (!group->name.empty()) ) ? group->name
                                             : std::string("the group") ) +
                    " and the groups within it";

    if (count == 0)
    {
        status(what + (every ? " was " : " were ") + state + " already",
               false);
    }
    else
    {
        status(what + (every ? " is " : " are ") + state + " (" +
                   format_dec(static_cast<u64>(count)) +
                   ((count == 1) ? " group " : " groups ") +
                   (_open ? "opened)" : "closed)"),
               false);
    }

    return;
}

void
app::load_sheet_into(
    sheet              _sheet,
    const std::string& _path
)
{
    doc().doc = std::move(_sheet);
    doc().steps.clear();
    doc().ui.selection.clear();
    doc().ui.grid_rect_rows.clear();
    doc().ui.grid_rect_low  = -1;
    doc().ui.grid_rect_high = -1;
    doc().ui.path           = _path;
    doc().ui.file           = _path;
    doc().ui.definition     = -1;
    doc().ui.dirty          = false;
    doc().ui.pristine       = false;
    doc().ui.browsing       = false;
    doc().ui.import_name.clear();
    doc().ui.import_notes.clear();

    // a sheet just opened is shown: its window comes to the front
    if (workspace* holder = holding(doc().id))
    {
        holder->raise = doc().id;
    }

    changed();
    shell_changed();

    return;
}

/*
frame
  The interface is realized again only when the document in front, or the
window around it, has moved on; the style is set again only when the theme
or accent has. The window is one host, filling the screen under its menu
bar, whose dock space the documents' and the tool windows dock in.
*/
void
app::frame()
{
    // the renderers draw this window's documents
    if (drawing() != this)
    {
        register_renderers([this](int _id) -> model*
                           {
                               return (_id == 0) ? m_doc
                                                 : find_document(_id);
                           },
                           [this]() -> const std::vector<action_entry>*
                           {
                               return &m_shell.actions;
                           });
        drawing() = this;
    }

    // the windows of documents and workspaces closed, forgotten: before
    // any window is drawn, so none is drawn half forgotten
    for (const std::string& each : m_forget)
    {
        ImGui::ClearWindowSettings(each.c_str());
    }

    m_forget.clear();

    // events queued from outside the frame go first
    if (!m_queue.empty())
    {
        std::vector<event> queued;
        queued.swap(m_queue);

        for (const event& each : queued)
        {
            apply(each);
        }
    }

    const int style =
        m_shell.theme * 1000003 +
        m_shell.scheme * 7919 +
        static_cast<int>(doc().shown.colours.accent & 0xFFFFFu);

    if (style != m_styled)
    {
        apply_style(doc().shown, m_shell.scheme);
        m_styled = style;
    }

    // the caches the interface draws from, if a change dropped them
    if (!doc().preview_valid)
    {
        wide operand;

        (void)sheet_number(doc().ui.operand, doc().doc.width, operand);

        doc().preview = preview_op(
            doc().doc,
            doc().res,
            op_targets(doc()),
            static_cast<op_kind>(std::clamp(doc().ui.op, 0, OP_COUNT - 1)),
            operand,
            doc().ui.op_probe);
        doc().preview_valid = true;
    }

    if (!doc().header_valid)
    {
        doc().header       = generate(doc().doc,
                                        doc().res,
                                        doc().today);
        doc().header_valid = true;
    }

    // what the preferences say the documents' rows end with
    for (const model* each : documents())
    {
        model* held = find_document(each->id);

        if (held != nullptr)
        {
            held->ui.child_offer = static_cast<int>(m_shell.prefs.child);
            held->ui.child_fill  = m_shell.prefs.child_fill;
        }
    }

    if ( (m_built != doc().version)       ||
         (m_built_doc != doc().id)        ||
         (m_built_shell != m_shell.version) )
    {
        std::vector<const model*> here;

        for (int id : current().documents)
        {
            const model* found = document(id);

            if (found != nullptr)
            {
                here.push_back(found);
            }
        }

        m_blueprint   = uxoxo::realize<uxoxo::platform::imgui_platform>(
            build_ui(shell_view{ m_shell, doc(), here, documents() }));
        m_built       = doc().version;
        m_built_doc   = doc().id;
        m_built_shell = m_shell.version;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("bitwisedom",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration          |
                 ImGuiWindowFlags_NoMove                |
                 ImGuiWindowFlags_NoDocking             |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoNavFocus            |
                 ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    for (const std::unique_ptr<model>& each : m_docs)
    {
        each->ui.hover_row = 0;
        each->ui.hover_bit = -1;
    }

    uxoxo::platform::imgui_frame posted;

    if (m_blueprint.draw)
    {
        m_blueprint.draw(posted);
    }

    shortcuts(posted);
    save_as_dialog();
    open_dialog();
    import_definitions_dialog();
    help_dialog();
    close_dialog();
    naming_dialog();
    ImGui::End();

    if (m_shell.demo)
    {
        ImGui::ShowDemoWindow(&m_shell.demo);
    }

    // a menu opened, a field focused, the columns fitted, a window brought
    // to the front, documents docked afresh and a layout built again are
    // so for one frame only; an arrangement, until it is made
    const bool drawn = ( (m_built == doc().version)          &&
                         (m_built_doc == doc().id)           &&
                         (m_built_shell == m_shell.version) );

    if ( (drawn) &&
         ( (doc().ui.menu_open)         ||
           (doc().ui.rename_focus)      ||
           (doc().ui.adding_focus)      ||
           (doc().ui.file_rename_focus) ||
           (doc().ui.definition_raise)  ||
           (doc().ui.fit_columns) ) )
    {
        doc().ui.menu_open         = false;
        doc().ui.rename_focus      = false;
        doc().ui.adding_focus      = false;
        doc().ui.file_rename_focus = false;
        doc().ui.definition_raise  = false;
        doc().ui.fit_columns       = false;
        ++doc().version;
    }

    workspace& here = current();
    const bool asked = ( (!here.placed.empty())                      ||
                         (here.reset)                                ||
                         (here.raise != 0)                           ||
                         (std::find(here.focus.begin(),
                                    here.focus.end(),
                                    true) != here.focus.end()) );

    if ( (drawn) &&
         ( (asked) ||
           (m_shell.node_raise) ) )
    {
        here.placed.clear();
        here.reset   = false;
        here.raise   = 0;
        here.focus.fill(false);
        m_shell.node_raise = false;
        shell_changed();
    }

    for (const event& each : posted.events)
    {
        apply(each);
    }

    if (ImGui::GetIO().WantSaveIniSettings)
    {
        keep_preferences();
    }

    keep_session(false);

    return;
}

void
app::shortcuts(
    uxoxo::platform::imgui_frame& _frame
)
{
    // typing into a field owns the keyboard
    if (ImGui::GetIO().WantTextInput)
    {
        return;
    }

    // chord
    //   posts an action when its key chord is pressed.
    auto chord = [&_frame](ImGuiKeyChord _keys,
                           const char*   _action)
    {
        if (ImGui::IsKeyChordPressed(_keys))
        {
            _frame.post(_action, option_set{});
        }

        return;
    };

    chord(ImGuiMod_Ctrl | ImGuiKey_Z, "edit.undo");
    chord(ImGuiMod_Ctrl | ImGuiKey_Y, "edit.redo");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, "edit.redo");
    chord(ImGuiMod_Ctrl | ImGuiKey_S, "file.save");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, "file.save_as");
    chord(ImGuiMod_Ctrl | ImGuiKey_O, "file.open");
    chord(ImGuiMod_Ctrl | ImGuiKey_E, "file.export");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E,
          "file.export_everything");
    chord(ImGuiMod_Ctrl | ImGuiKey_D, "edit.duplicate");
    chord(ImGuiKey_Delete, "edit.delete");
    chord(ImGuiMod_Ctrl | ImGuiKey_G, "edit.add_group");
    chord(ImGuiMod_Ctrl | ImGuiKey_B, "edit.add_bit");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_A, "node.open");
    chord(ImGuiMod_Alt | ImGuiKey_UpArrow, "edit.move_up");
    chord(ImGuiMod_Alt | ImGuiKey_DownArrow, "edit.move_down");
    chord(ImGuiMod_Ctrl | ImGuiKey_J, "panel.toggle");

    // the documents and the window around them: a new sheet, the one in
    // front closed, the window in front zoomed as a browser's page is, the
    // Options, and the workspace beside this one
    chord(ImGuiMod_Ctrl | ImGuiKey_N, "file.new");
    chord(ImGuiMod_Ctrl | ImGuiKey_W, "doc.close");
    chord(ImGuiMod_Ctrl | ImGuiKey_Equal, "view.zoom_in");
    chord(ImGuiMod_Ctrl | ImGuiKey_Minus, "view.zoom_out");
    chord(ImGuiMod_Ctrl | ImGuiKey_0, "view.zoom_reset");
    chord(ImGuiMod_Ctrl | ImGuiKey_Comma, "options.open");
    chord(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_RightArrow,
          "workspace.next");
    chord(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_LeftArrow,
          "workspace.previous");

    // every group opened or closed: Ctrl+Shift with the main keyboard's
    // plus or minus, or Ctrl with the keypad's
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Equal, "tree.open_all");
    chord(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd, "tree.open_all");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Minus, "tree.close_all");
    chord(ImGuiMod_Ctrl | ImGuiKey_KeypadSubtract, "tree.close_all");

    // the selected row: renamed, copied as code, or code pasted beside it
    const long selected = doc().ui.selection.empty()
                              ? 0L
                              : static_cast<long>(
                                    doc().ui.selection.front());

    // with_row
    //   posts an action about the selected row when its chord is pressed.
    auto with_row = [&_frame, selected](ImGuiKeyChord _keys,
                                        const char*   _action)
    {
        if (ImGui::IsKeyChordPressed(_keys))
        {
            option_set payload;
            payload.set("value", selected);
            _frame.post(_action, std::move(payload));
        }

        return;
    };

    if (selected != 0)
    {
        with_row(ImGuiKey_F2, "rename.tree");
    }

    with_row(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C, "code.copy");
    with_row(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V, "code.paste");

    if (ImGui::IsKeyChordPressed(ImGuiKey_G))
    {
        option_set payload;
        payload.set("value", !doc().doc.grid.lines);
        payload.set("target", "grid.lines");
        _frame.post("set", std::move(payload));
    }

    return;
}

// ------------------------------------------------------------------- apply

void
app::apply(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;
    const long         value   = payload.as_long("value", 0);

    // what the user did, for the console
    note_action(_event);

    // an event from a document's window is that document's: it comes to
    // the front first -- unless it has been closed since
    const long owner = payload.as_long("document", 0);

    if ( (owner != 0) &&
         (owner != doc().id) )
    {
        if (find_document(static_cast<int>(owner)) == nullptr)
        {
            return;
        }

        activate(static_cast<int>(owner), false);
    }

    // the window around the documents
    if (apply_shell(_event))
    {
        return;
    }

    // settings, by path: the window's own, or the sheet's
    if (kind == "set")
    {
        const std::string path  = payload.as_string("target", "");
        const setting*    found = find_setting(path);

        if (is_environment(path))
        {
            set_environment(payload);

            return;
        }

        if (found != nullptr)
        {
            // a cell width set by hand is the width the cells take: the
            // room to spare no longer widens them
            const bool by_hand = ( (path == "grid.cell_width") &&
                                   (doc().doc.grid.fill ==
                                    grid_fill::cells) );

            edit("set:" + path,
                 [found, &payload, by_hand](sheet& _sheet)
                 {
                     found->write(_sheet, payload);

                     if (by_hand)
                     {
                         _sheet.grid.fill = grid_fill::none;
                     }
                 });

            if (by_hand)
            {
                status("cells sized by hand: the room to spare goes to "
                       "nothing now (Grid lines, Room to spare goes to)",
                       false);
            }
        }

        return;
    }

    // a name, from the inspector, is a rename: its references follow
    if (kind == "row.name")
    {
        finish_rename(static_cast<int>(payload.as_long("target", 0)),
                      payload.as_string("value", ""));

        return;
    }

    if (kind.compare(0, 4, "row.") == 0)
    {
        row_edit(_event);

        return;
    }

    if (kind.compare(0, 8, "segment.") == 0)
    {
        segment_edit(_event);

        return;
    }

    if (kind.compare(0, 8, "display.") == 0)
    {
        display_edit(_event);

        return;
    }

    if (kind.compare(0, 8, "special.") == 0)
    {
        special_edit(_event);

        return;
    }

    if (kind.compare(0, 5, "node.") == 0)
    {
        node_edit(_event);

        return;
    }

    if (kind == "grid.sort_keep")
    {
        keep_sorted();

        return;
    }

    if (kind.compare(0, 9, "template.") == 0)
    {
        template_edit(_event);

        return;
    }

    // ------------------------------------------------------------ selection
    if (kind == "select")
    {
        select(static_cast<int>(value));
    }
    else if (kind == "toggle_open")
    {
        const int  id   = static_cast<int>(value);
        const bool open = payload.as_bool("open", true);

        quietly([id, open](sheet& _sheet)
                {
                    row* found = find_row(_sheet, id);

                    if (found != nullptr)
                    {
                        found->collapsed = !open;
                    }
                });
    }
    else if ( (kind == "tree.open_all") ||
              (kind == "tree.close_all") )
    {
        open_all(static_cast<int>(value), (kind == "tree.open_all"));
    }
    else if (kind == "drop")
    {
        drop_row(static_cast<int>(value),
                 static_cast<int>(payload.as_long("target", 0)));
    }
    // ----------------------------------------------------------------- edit
    else if (kind == "edit.undo")
    {
        if (doc().steps.undo(doc().doc))
        {
            changed();
            status("undone", false);
        }
    }
    else if (kind == "edit.redo")
    {
        if (doc().steps.redo(doc().doc))
        {
            changed();
            status("redone", false);
        }
    }
    else if (kind == "edit.add")
    {
        add_row(static_cast<int>(value));
    }
    else if (kind == "edit.add_bit")
    {
        add_row(static_cast<int>(def_kind::bit));
    }
    else if (kind == "edit.add_group")
    {
        add_row(-1);
    }
    else if (kind == "tree.add_child")
    {
        // the field it was named in has the keyboard back: the tree's, or
        // the grid's
        doc().ui.adding_where = payload.as_string("where", "tree");
        add_child(static_cast<int>(value), payload.as_string("text", ""));
    }
    else if (kind == "edit.delete")
    {
        delete_rows();
    }
    else if (kind == "edit.duplicate")
    {
        duplicate_rows();
    }
    else if (kind == "edit.move_up")
    {
        move_row_by(-1);
    }
    else if (kind == "edit.move_down")
    {
        move_row_by(1);
    }
    // ----------------------------------------------------------------- grid
    else if (kind == "grid.toggle")
    {
        grid_toggle(static_cast<int>(payload.as_long("target", 0)),
                    static_cast<int>(value));
    }
    else if (kind == "grid.rectangle")
    {
        const int from = static_cast<int>(payload.as_long("target", 0));
        const int to = static_cast<int>(payload.as_long("last", 0));
        const std::vector<int> order = visible_order(doc());
        const auto first = std::find(order.begin(), order.end(), from);
        const auto last  = std::find(order.begin(), order.end(), to);
        if ((first != order.end()) && (last != order.end()))
        {
            doc().ui.grid_rect_rows.assign(
                 std::min(first, last), std::max(first, last) + 1);
            doc().ui.selection = doc().ui.grid_rect_rows;
            doc().ui.grid_rect_low = static_cast<int>(
                 payload.as_long("low", 0));
            doc().ui.grid_rect_high = static_cast<int>(
                 payload.as_long("high", 0));
            doc().ui.anchor = from;
            doc().preview_valid = false;
            ++doc().version;
        }
    }
    else if (kind == "grid.span")
    {
        grid_span(static_cast<int>(payload.as_long("target", 0)),
                  static_cast<int>(payload.as_long("low", 0)),
                  static_cast<int>(payload.as_long("high", 0)),
                  payload.as_bool("set", true));
    }
    else if (kind == "grid.probe")
    {
        const int bit = static_cast<int>(value);

        quietly([bit](sheet& _sheet)
                {
                    _sheet.probe.set(bit, !_sheet.probe.test(bit));
                });
    }
    else if (kind == "grid.segment")
    {
        collapse_segment(payload);
    }
    else if (kind == "grid.resize")
    {
        const std::string column = payload.as_string("target", "name");
        const float       delta  =
            static_cast<float>(as_number(payload, "value", 0.0));

        quietly([column, delta](sheet& _sheet)
                {
                    const bool values = (column == "value");
                    float&     width  = values ? _sheet.grid.value_width
                                               : _sheet.grid.name_width;

                    // a drag narrows the value column, but does not hide
                    // it: a hidden column has no edge to drag it back by
                    width = within_setting(values ? "grid.value_width"
                                                  : "grid.name_width",
                                           width + delta);

                    if (values)
                    {
                        width = std::max(width, VALUE_FLOOR);
                    }
                });
    }
    else if (kind == "colours.automatic")
    {
        edit("colours.automatic",
             [](sheet& _sheet)
             {
                 automatic_colours(_sheet);
             });
        status("the sheet's colours are the scheme's again", false);
    }
    // ---------------------------------------------------------- operations
    else if (kind == "ops.op")
    {
        doc().ui.op         = static_cast<int>(value);
        doc().preview_valid = false;
        ++doc().version;
    }
    else if (kind == "ops.operand")
    {
        doc().ui.operand    = payload.as_string("value", "");
        doc().preview_valid = false;
        ++doc().version;
    }
    else if (kind == "ops.probe")
    {
        doc().ui.op_probe   = payload.as_bool("value", false);
        doc().preview_valid = false;
        ++doc().version;
    }
    else if (kind == "ops.preview")
    {
        doc().ui.op_preview = payload.as_bool("value", true);
        ++doc().version;
    }
    else if (kind == "ops.apply")
    {
        apply_operation();
    }
    // ---------------------------------------------------------------- probe
    else if (kind == "probe.text")
    {
        wide bits;

        if (sheet_number(payload.as_string("value", ""),
                         doc().doc.width,
                         bits))
        {
            quietly([&bits](sheet& _sheet) { _sheet.probe = bits; });
        }
    }
    else if (kind == "probe.clear")
    {
        quietly([](sheet& _sheet) { _sheet.probe = wide(); });
    }
    else if (kind == "probe.row")
    {
        const resolved_row* found = doc().res.find(static_cast<int>(value));

        if ( (found != nullptr) &&
             (found->has_value) )
        {
            const wide bits = found->bits;

            quietly([&bits](sheet& _sheet) { _sheet.probe = bits; });
        }
    }
    // ------------------------------------------------------------ the files
    else if (kind == "file.path")
    {
        doc().ui.path = payload.as_string("value", doc().ui.path);
        ++doc().version;
    }
    // the sheet's file renamed in its tab: the name typed over the tab
    else if (kind == "doc.rename")
    {
        doc().ui.file_renaming     = true;
        doc().ui.file_rename_focus = true;
        ++doc().version;
    }
    else if (kind == "doc.rename.cancel")
    {
        doc().ui.file_renaming = false;
        ++doc().version;
    }
    else if (kind == "doc.rename.commit")
    {
        rename_file(payload.as_string("text", ""));
    }
    else if (kind == "file.new")
    {
        // a sheet of its own, beside the others
        (void)open_document();
        load_sheet_into(make_document("untitled", 32),
                        "untitled.bitwisedom.json");
        doc().ui.selection.assign(1, doc().doc.root.children.front().id);
        doc().ui.anchor   = doc().ui.selection.front();
        doc().ui.pristine = true;
        ++doc().version;
        status("a new sheet", false);
    }
    else if (kind == "file.example")
    {
        const std::vector<std::string> names = example_names();
        const std::size_t              index =
            static_cast<std::size_t>(std::clamp<long>(
                value, 0, static_cast<long>(names.size()) - 1));

        // in place of an example only looked at, or a new sheet untouched;
        // beside the rest
        (void)fresh_document();
        load_sheet_into(make_example(names[index]),
                        names[index] + ".bitwisedom.json");
        doc().ui.fit_columns = true;
        doc().ui.browsing    = true;
        status("the " + names[index] + " example", false);
    }
    else if (kind == "file.open")
    {
        // the file named; or, with none, the dialog that asks for one,
        // starting in the folder of the sheet in front
        const std::string asked = payload.as_string("value", "");

        if (!asked.empty())
        {
            open_file(asked);
        }
        else
        {
            const std::string& path  = doc().ui.path;
            const std::size_t  slash = path.find_last_of("/\\");

            m_open_path  = (slash == std::string::npos)
                               ? std::string()
                               : path.substr(0, slash + 1);
            m_open_open  = true;
            m_open_focus = true;
        }
    }
    else if (kind == "definitions.import")
    {
        m_import_open = true;
    }
    else if (kind == "definitions.remove")
    {
        // the file named, or the one last in front
        const long index = payload.has("value") ? value
                                                : doc().ui.definition;

        if ( (index >= 0) &&
             (static_cast<std::size_t>(index) <
              doc().doc.definitions.size()) )
        {
            edit("definitions.remove",
                 [index](sheet& _sheet)
                 {
                     _sheet.definitions.erase(_sheet.definitions.begin() +
                                              index);
                 });
            doc().ui.definition = -1;
            status("removed the imported definition file", false);
        }
    }
    else if (kind == "definitions.front")
    {
        // a definition file's window came to the front
        doc().ui.definition = static_cast<int>(value);
    }
    else if (kind == "definitions.show")
    {
        // a definition file's window brought to the front
        if ( (value >= 0) &&
             (static_cast<std::size_t>(value) <
              doc().doc.definitions.size()) )
        {
            doc().ui.definition       = static_cast<int>(value);
            doc().ui.definition_raise = true;
            ++doc().version;
        }
    }
    else if (kind == "definitions.edit")
    {
        // a definition file's text as an editor, or back as a view
        const int index = static_cast<int>(value);

        doc().ui.definition_editing =
            (doc().ui.definition_editing == index) ? -1 : index;
        ++doc().version;
    }
    else if (kind == "definitions.text")
    {
        // what is typed into a definition file: one step to undo while
        // the typing goes on
        const std::size_t index = static_cast<std::size_t>(value);
        const std::string text  = payload.as_string("text", "");

        if (index < doc().doc.definitions.size())
        {
            edit("definitions.text:" + format_dec(static_cast<u64>(index)),
                 [index, &text](sheet& _sheet)
                 {
                     _sheet.definitions[index].text = text;
                 });
        }
    }
    else if (kind == "definitions.toggle")
    {
        const std::size_t index = static_cast<std::size_t>(value);
        if (index < doc().doc.definitions.size())
            edit("definitions.toggle", [index](sheet& s) {
                s.definitions[index].show_other =
                    !s.definitions[index].show_other;
            });
    }
    else if (kind == "context.add")
    {
        doc().ui.menu_kind = "sheet";
        doc().ui.menu_where = "add";
        doc().ui.menu_open = true;
        ++doc().version;
    }
    else if (kind == "file.save")
    {
        save_file();
    }
    else if (kind == "file.save_as")
    {
        m_save_as_path = doc().ui.path;
        m_save_as_extension =
            (doc().doc.gen.file_name.size() >= 4 &&
             doc().doc.gen.file_name.substr(
                 doc().doc.gen.file_name.size() - 4) == ".hpp") ? 1 : 0;
        m_save_as_open = true;
        m_save_as_focus = true;
    }
    else if (kind == "file.export")
    {
        export_header();
    }
    else if (kind == "file.export_bitfield")
    {
        export_json(false);
    }
    else if (kind == "file.export_everything")
    {
        export_json(true);
    }
    else if (kind == "json.copy")
    {
        ImGui::SetClipboardText(write_bitfield(doc().doc).c_str());
        status("the bitfield is on the clipboard, as JSON", false);
    }
    else if (kind == "gen.copy")
    {
        const generated header = generate(doc().doc,
                                          doc().res,
                                          doc().today);

        ImGui::SetClipboardText(header.text.c_str());
        status("the header is on the clipboard", false);
    }
    // ------------------------------------------- the menu and renames
    else if ( (kind == "context.tree")  ||
              (kind == "context.grid")  ||
              (kind == "context.table") )
    {
        open_menu("row", kind.substr(8), payload);
    }
    else if (kind == "context.segment")
    {
        open_menu("segment", "grid", payload);
    }
    else if (kind == "context.sheet")
    {
        open_menu("sheet", "grid", payload);
    }
    else if (kind == "context.names_by")
    {
        open_menu("names_by", "grid", payload);
    }
    else if (kind == "context.columns")
    {
        open_menu("columns", "grid", payload);
    }
    else if (kind == "context.special")
    {
        open_menu("special", "grid", payload);
    }
    else if ( (kind == "rename.tree")  ||
              (kind == "rename.grid")  ||
              (kind == "rename.table") )
    {
        begin_rename(static_cast<int>(value),
                     (kind == "rename.grid") ? "grid" : "tree");
    }
    else if (kind == "rename.commit")
    {
        finish_rename(static_cast<int>(value), payload.as_string("text", ""));
    }
    else if (kind == "rename.cancel")
    {
        doc().ui.renaming         = 0;
        doc().ui.renaming_segment = -1;
        ++doc().version;
    }
    else if (kind == "rename.segment")
    {
        begin_segment_rename(payload);
    }
    else if (kind == "rename.segment.commit")
    {
        finish_segment_rename(payload.as_string("text", ""));
    }
    // ---------------------------------------------------- code in and out
    else if (kind == "code.copy")
    {
        copy_code(static_cast<int>(value));
    }
    else if (kind == "code.paste")
    {
        paste_code(static_cast<int>(value));
    }
    else if (kind == "code.new")
    {
        const char*       clipboard = ImGui::GetClipboardText();
        const std::string text      = (clipboard != nullptr) ? clipboard
                                                             : "";

        // JSON -- a sheet, a bitfield or an export -- or code
        if (looks_like_sheet(text))
        {
            import_json_text(std::string(), text);
        }
        else
        {
            import_code_text(std::string(), text);
        }
    }
    else if (kind == "file.drop")
    {
        open_file(payload.as_string("value", ""));
    }
    // --------------------------------------------------------- the grid's
    else if (kind == "grid.width")
    {
        grid_width(payload);
    }
    else if (kind == "grid.swap_text")
    {
        edit("grid.order", [](sheet& s)
        {
            s.grid.values_first = !s.grid.values_first;
        });
    }
    // ----------------------------------------------------------------- help
    else if (kind == "help.show")
    {
        m_help      = (value == 1) ? 1 : 0;
        m_help_open = true;
    }

    return;
}

// --------------------------------------------------------------- the shell

/*
apply_shell
  The events about the window around the documents: the documents
themselves, brought to the front and closed; the workspaces; the windows,
their arrangement and their zoom; the tool windows; the look; the Options
window and the settings files. True when the event was one of these.
*/
bool
app::apply_shell(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;
    const long         value   = payload.as_long("value", 0);
    const long         count   = static_cast<long>(m_shell.workspaces.size());

    // ------------------------------------------------------------ documents
    if (kind == "doc.activate")
    {
        // a document's window the user brought to the front, or one asked
        // for from a menu, whose window comes to the front too
        activate(static_cast<int>(value), !payload.as_bool("clicked", false));
    }
    else if (kind == "doc.close")
    {
        close_document((value == 0) ? doc().id : static_cast<int>(value),
                       false);
    }
    else if (kind == "doc.close_all")
    {
        // every document in the workspace; the first with changes asks,
        // and the rest wait for another Close all
        const std::vector<int> here = current().documents;

        for (int id : here)
        {
            close_document(id, false);

            if (m_closing != 0)
            {
                break;
            }
        }
    }
    else if (kind == "doc.discard")
    {
        // closed, its changes lost: the dialog's "Don't save"
        close_document((value == 0) ? doc().id : static_cast<int>(value),
                       true);
    }
    // ----------------------------------------------------------- workspaces
    else if (kind == "workspace.new")
    {
        (void)add_workspace();
        to_workspace(m_shell.workspaces.size() - 1);
    }
    else if (kind == "workspace.switch")
    {
        // the combo's last choice is a new workspace
        if ( (value >= 0) &&
             (value < count) )
        {
            to_workspace(static_cast<std::size_t>(value));
        }
        else if (value == count)
        {
            (void)add_workspace();
            to_workspace(static_cast<std::size_t>(count));
        }
    }
    else if ( (kind == "workspace.next") ||
              (kind == "workspace.previous") )
    {
        const long step = (kind == "workspace.next") ? 1L : count - 1L;

        to_workspace(static_cast<std::size_t>(
            (static_cast<long>(m_shell.current) + step) % count));
    }
    else if (kind == "workspace.close")
    {
        close_workspace(payload.has("value")
                            ? static_cast<std::size_t>(std::max(0L, value))
                            : m_shell.current);
    }
    else if (kind == "workspace.naming")
    {
        m_naming_open = true;
        m_naming_text = current().name;
    }
    else if (kind == "workspace.rename")
    {
        const std::string text  = payload.as_string("value", "");
        const std::size_t first = text.find_first_not_of(" \t\r\n");
        const std::size_t last  = text.find_last_not_of(" \t\r\n");

        if (first != std::string::npos)
        {
            current().name = text.substr(first, last - first + 1);
            shell_changed();
            status("the workspace is " + current().name, false);
        }
    }
    else if (kind == "workspace.move")
    {
        // a document -- the one in front, for none -- to a workspace; past
        // the last, to a new one
        const long target = payload.as_long("target", 0);
        const int  moved  = (target == 0) ? doc().id
                                          : static_cast<int>(target);

        if (value == count)
        {
            (void)add_workspace();
        }

        move_document(moved,
                      static_cast<std::size_t>(std::clamp(
                          value, 0L,
                          static_cast<long>(m_shell.workspaces.size()) - 1L)));
    }
    // -------------------------------------------------------------- windows
    else if (kind == "window.arrange")
    {
        static const char* const WAYS[ARRANGEMENT_COUNT] = {
            "as tabs of one window", "side by side, in columns",
            "one above another, in rows", "in a grid" };
        const int way = static_cast<int>(
            std::clamp(value, 0L, static_cast<long>(ARRANGEMENT_COUNT - 1)));

        current().arrange = way;
        shell_changed();
        status("the sheets are laid out " + std::string(WAYS[way]), false);
    }
    else if (kind == "window.arranged")
    {
        // the dock space laid the documents out: the request is done
        current().arrange = -1;
        shell_changed();
    }
    else if (kind == "window.reset_layout")
    {
        current().reset = true;
        shell_changed();
        status("the windows are where a new workspace has them", false);
    }
    else if (kind == "window.focus")
    {
        // the window the zoom keys act on; nothing to draw again
        m_shell.focused = payload.as_string("target", "");
    }
    // ---------------------------------------------------------- tool windows
    else if ( (kind == "tool.open")  ||
              (kind == "tool.close") ||
              (kind == "tool.toggle") )
    {
        const int  index = static_cast<int>(
            std::clamp(value, 0L, static_cast<long>(TOOL_COUNT - 1)));
        const bool open  =
            (kind == "tool.open") ||
            ( (kind == "tool.toggle") &&
              (!current().tools[static_cast<std::size_t>(index)]) );

        tool_to(static_cast<tool>(index), open, open);
    }
    else if (kind == "tab.show")
    {
        // a page of the old bottom panel, by its old number: its window
        tool_to(tool_for_page(static_cast<int>(value)), true, true);
    }
    else if (kind == "panel.toggle")
    {
        // the tool windows below the documents: put away, or back
        workspace& here = current();
        bool       any  = false;

        for (int i = 0; i < TOOL_COUNT; ++i)
        {
            any = ( (any) ||
                    ( (tool_slot(static_cast<tool>(i)) == dock_slot::bottom) &&
                      (here.tools[static_cast<std::size_t>(i)]) ) );
        }

        if (any)
        {
            here.stowed.clear();

            for (int i = 0; i < TOOL_COUNT; ++i)
            {
                std::size_t at = static_cast<std::size_t>(i);

                if ( (tool_slot(static_cast<tool>(i)) == dock_slot::bottom) &&
                     (here.tools[at]) )
                {
                    here.stowed.push_back(i);
                    here.tools[at] = false;
                }
            }
        }
        else
        {
            for (int i = 0; i < TOOL_COUNT; ++i)
            {
                const tool each   = static_cast<tool>(i);
                const bool stowed = (std::find(here.stowed.begin(),
                                               here.stowed.end(),
                                               i) != here.stowed.end());

                if ( (tool_slot(each) == dock_slot::bottom) &&
                     ( (stowed) ||
                       ( (here.stowed.empty()) &&
                         (tool_open(each)) ) ) )
                {
                    here.tools[static_cast<std::size_t>(i)] = true;
                }
            }

            here.stowed.clear();
        }

        shell_changed();
    }
    else if ( (kind == "view.symbols") ||
              (kind == "view.inspector") )
    {
        const tool which = (kind == "view.symbols") ? tool::symbols
                                                    : tool::inspector;
        const bool open  = payload.as_bool(
            "value",
            !current().tools[static_cast<std::size_t>(which)]);

        tool_to(which, open, open);
    }
    // ----------------------------------------------------------------- zoom
    else if (kind == "view.zoom")
    {
        const std::string key = payload.as_string("target", "");

        zoom(key.empty() ? zoom_key() : key,
             static_cast<float>(value) * ZOOM_STEP);
    }
    else if (kind == "view.zoom_in")
    {
        zoom(zoom_key(), ZOOM_STEP);
    }
    else if (kind == "view.zoom_out")
    {
        zoom(zoom_key(), -ZOOM_STEP);
    }
    else if (kind == "view.zoom_reset")
    {
        zoom(zoom_key(), 0.0f);
    }
    else if (kind == "view.zoom_reset_all")
    {
        m_shell.zoom.clear();
        shell_changed();
        status("every window at 100%", false);
    }
    // ------------------------------------------------------------- the look
    else if ( (kind == "view.theme") ||
              (kind == "view.scheme") )
    {
        option_set made;

        made.set("target", std::string((kind == "view.theme")
                                           ? "environment.theme"
                                           : "environment.scheme"));
        made.set("value", value);
        set_environment(made);
    }
    // ------------------------------------------------------------------ app
    else if (kind == "app.quit")
    {
        m_shell.quit = true;
    }
    else if (kind == "app.demo")
    {
        m_shell.demo = payload.as_bool("value", !m_shell.demo);
        shell_changed();
    }
    // -------------------------------------------------------------- options
    else if (kind == "options.open")
    {
        m_shell.options_open = true;

        if (payload.has("value"))
        {
            m_shell.options_page = static_cast<int>(value);
        }

        shell_changed();
    }
    else if (kind == "options.close")
    {
        m_shell.options_open = false;
        shell_changed();
    }
    else if (kind == "options.page")
    {
        m_shell.options_page = static_cast<int>(value);
        m_shell.options_search.clear();
        shell_changed();
    }
    else if (kind == "options.search")
    {
        m_shell.options_search = payload.as_string("value", "");
        shell_changed();
    }
    else if (kind.compare(0, 9, "settings.") == 0)
    {
        settings_file(kind, payload);
    }
    else
    {
        return false;
    }

    return true;
}

/*
set_environment
  One of the window's own settings, set as its control posted it -- and
what follows from it: every sheet drawn in a new theme or scheme, the
symbols tree's window opened or closed in every workspace. The settings
file keeps it.
*/
void
app::set_environment(
    const option_set& _payload
)
{
    const std::string  path  = _payload.as_string("target", "");
    const env_setting* found = find_env_setting(path);

    if (found == nullptr)
    {
        return;
    }

    const int  theme  = m_shell.theme;
    const int  scheme = m_shell.scheme;
    const bool merged = m_shell.prefs.merged;

    found->write(m_shell, _payload);

    if ( (m_shell.theme != theme) ||
         (m_shell.scheme != scheme) )
    {
        restyle();
    }

    if (m_shell.prefs.merged != merged)
    {
        for (workspace& each : m_shell.workspaces)
        {
            each.tools[static_cast<std::size_t>(tool::symbols)] =
                !m_shell.prefs.merged;
        }
    }

    remember(m_shell);
    shell_changed();

    return;
}

/*
restyle
  Every document drawn again in the window's theme and colour scheme.
*/
void
app::restyle()
{
    model* const front = m_doc;

    for (const std::unique_ptr<model>& each : m_docs)
    {
        m_doc = each.get();
        changed();
    }

    m_doc = front;

    return;
}

/*
zoom_key
  The window the zoom keys and menu act on: the one last in front, if it is
still one of this workspace's -- else the document in front.
*/
std::string
app::zoom_key() const
{
    const workspace&  here   = m_shell.workspaces[m_shell.current];
    const std::string prefix = "w" + format_dec(static_cast<u64>(here.id)) +
                               ".";
    const std::string& last  = m_shell.focused;

    if (last.compare(0, prefix.size(), prefix) == 0)
    {
        return last;
    }

    for (int id : here.documents)
    {
        if (last == document_key(id))
        {
            return last;
        }
    }

    return document_key(m_doc->id);
}

/*
settings_file
  The settings files: the window's settings and the sheet's preferences
written to one, read from one and applied, and the files the window keeps
a list of -- applied again, forgotten, or deleted. A file is kept on the
list while it is where it was written or read.
*/
void
app::settings_file(
    const std::string& _kind,
    const option_set&  _payload
)
{
    std::vector<std::string>& files = m_shell.prefs.settings_files;
    const long                index = _payload.as_long("value", -1);
    const bool                listed =
        ( (index >= 0) &&
          (static_cast<std::size_t>(index) < files.size()) );

    // keep
    //   a file put first on the list.
    auto keep = [&files](const std::string& _path)
    {
        files.erase(std::remove(files.begin(), files.end(), _path),
                    files.end());
        files.insert(files.begin(), _path);

        return;
    };

    if (_kind == "settings.path")
    {
        m_shell.settings_path = _payload.as_string("value", "");
        shell_changed();

        return;
    }

    if (_kind == "settings.export")
    {
        std::string path = _payload.as_string("value", m_shell.settings_path);
        std::string error;

        if (path.empty())
        {
            status("name the settings file first", true);

            return;
        }

        if (!write_file(path, write_settings(m_shell, doc().doc), error))
        {
            status(error, true);

            return;
        }

        keep(path);
        remember(m_shell);
        shell_changed();
        status("the settings are in " + path, false);

        return;
    }

    if ( (_kind == "settings.import") ||
         (_kind == "settings.apply") )
    {
        const std::string path =
            (_kind == "settings.apply")
                ? (listed ? files[static_cast<std::size_t>(index)]
                          : std::string())
                : _payload.as_string("value", m_shell.settings_path);
        std::string                text;
        std::string                error;
        std::vector<setting_value> values;

        if (path.empty())
        {
            status("name the settings file first", true);

            return;
        }

        if ( (!read_file(path, text, error)) ||
             (!read_settings(text, values, error)) )
        {
            status(path + ": " + error, true);

            return;
        }

        int sheet_values = 0;

        for (const setting_value& each : values)
        {
            if (is_environment(each.path))
            {
                option_set made = each.payload;

                made.set("target", each.path);
                set_environment(made);
            }
            else
            {
                ++sheet_values;
            }
        }

        // the sheet's, one step to undo
        if (sheet_values > 0)
        {
            edit(std::string(),
                 [&values](sheet& _sheet)
                 {
                     for (const setting_value& each : values)
                     {
                         const setting* found = find_setting(each.path);

                         if ( (found != nullptr) &&
                              (!is_environment(each.path)) )
                         {
                             found->write(_sheet, each.payload);
                         }
                     }
                 });
        }

        keep(path);
        remember(m_shell);
        shell_changed();
        status("applied " + format_dec(values.size()) + " setting(s) from " +
                   path,
               false);

        return;
    }

    if ( (_kind == "settings.forget") &&
         (listed) )
    {
        const std::string path = files[static_cast<std::size_t>(index)];

        files.erase(files.begin() + index);
        remember(m_shell);
        shell_changed();
        status(path + " is off the list, and still where it was", false);

        return;
    }

    if ( (_kind == "settings.delete") &&
         (listed) )
    {
        namespace fs = std::filesystem;

        const std::string path = files[static_cast<std::size_t>(index)];
        std::error_code   failed;

        fs::remove(utf8_path(path), failed);
        files.erase(files.begin() + index);
        remember(m_shell);
        shell_changed();

        if (failed)
        {
            status(path + " is off the list, but was not deleted: " +
                       failed.message(),
                   true);
        }
        else
        {
            status("deleted " + path, false);
        }

        return;
    }

    return;
}

/*
fresh_document
  Where something opened goes: the document in front, if it is a new sheet
never touched or an example only looked at -- else a new document beside
it.
*/
model&
app::fresh_document()
{
    if ( ( (doc().ui.pristine) ||
           (doc().ui.browsing) ) &&
         (!doc().ui.dirty) )
    {
        return doc();
    }

    return open_document();
}

/*
close_workspace
  A workspace closed: its documents go to the workspace before it -- or
after it, for the first -- docked among the documents there, which comes
to the front. The window keeps one workspace at least.
*/
void
app::close_workspace(
    std::size_t _index
)
{
    if ( (m_shell.workspaces.size() < 2) ||
         (_index >= m_shell.workspaces.size()) )
    {
        status("the window keeps one workspace at least", true);

        return;
    }

    const workspace   closing = m_shell.workspaces[_index];
    const std::size_t into    = (_index > 0) ? _index - 1 : 1;

    {
        workspace& next = m_shell.workspaces[into];

        for (int id : closing.documents)
        {
            next.documents.push_back(id);
            next.placed.push_back(id);
        }

        if ( (closing.active != 0) &&
             (!closing.documents.empty()) )
        {
            next.active = closing.active;
        }
    }

    // its tool windows are forgotten with it
    for (int i = 0; i < TOOL_COUNT; ++i)
    {
        m_forget.push_back("###" + tool_window_key(closing.id,
                                                   static_cast<tool>(i)));
    }

    m_shell.workspaces.erase(m_shell.workspaces.begin() +
                             static_cast<std::ptrdiff_t>(_index));

    const std::size_t now  = (into > _index) ? into - 1 : into;
    const workspace&  here = m_shell.workspaces[now];

    m_shell.current = now;
    m_doc           = nullptr;

    if (here.documents.empty())
    {
        to_workspace(now);
    }
    else
    {
        activate( (find_document(here.active) != nullptr)
                      ? here.active
                      : here.documents.back() );
    }

    status("closed " + closing.name +
               (closing.documents.empty()
                    ? std::string()
                    : "; its sheets are in " + current().name),
           false);
    shell_changed();

    return;
}

/*
close_dialog
  A sheet with changes the user asked to close: saved first, closed with
its changes lost, or kept open.
*/
void
app::close_dialog()
{
    const char* const title = "Close the sheet?##bitwisedom";

    if (m_closing_open)
    {
        ImGui::OpenPopup(title);
        m_closing_open = false;
    }

    if (!ImGui::BeginPopupModal(title, nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    const model* closing = find_document(m_closing);

    if (closing == nullptr)
    {
        m_closing = 0;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();

        return;
    }

    const float width = 110.0f * ui_scale();

    ImGui::Text("%s has changes that are not saved.",
                closing->ui.path.c_str());
    ImGui::TextDisabled("Save them before it closes?");
    ImGui::Separator();

    if (ImGui::Button("Save", ImVec2(width, 0.0f)))
    {
        const int id = m_closing;

        activate(id);
        save_file();

        // saved: closed; not saved: open still, the status line says why
        if (!doc().ui.dirty)
        {
            close_document(id, true);
        }

        m_closing = 0;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Don't save", ImVec2(width, 0.0f)))
    {
        const int id = m_closing;

        m_closing = 0;
        close_document(id, true);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if ( (ImGui::Button("Cancel", ImVec2(width, 0.0f))) ||
         (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) )
    {
        m_closing = 0;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return;
}

/*
naming_dialog
  The workspace in front, given a name: Enter or OK names it, Escape or
Cancel leaves it as it was.
*/
void
app::naming_dialog()
{
    const char* const title = "Name the workspace##bitwisedom";
    bool              focus = false;

    if (m_naming_open)
    {
        ImGui::OpenPopup(title);
        m_naming_open = false;
        focus         = true;
    }

    if (!ImGui::BeginPopupModal(title, nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    const float width = 110.0f * ui_scale();

    if (focus)
    {
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::SetNextItemWidth(width * 3.0f);

    const bool entered = ImGui::InputText("##workspace.name",
                                          &m_naming_text,
                                          ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::Separator();

    if ( (ImGui::Button("OK", ImVec2(width, 0.0f))) ||
         (entered) )
    {
        option_set made;

        made.set("value", m_naming_text);
        post(event{ "workspace.rename", made });
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if ( (ImGui::Button("Cancel", ImVec2(width, 0.0f))) ||
         (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) )
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return;
}

// --------------------------------------------------------------- selection

/*
select
  A plain click selects one row; Ctrl adds or removes one; Shift selects the
visible rows from the anchor to this one.
*/
void
app::select(
    int _id
)
{
    const ImGuiIO&    io        = ImGui::GetIO();
    std::vector<int>& selection = doc().ui.selection;
    doc().ui.grid_rect_rows.clear();
    doc().ui.grid_rect_low = -1;
    doc().ui.grid_rect_high = -1;

    if ( (io.KeyShift) &&
         (doc().ui.anchor != 0) )
    {
        const std::vector<int> order = visible_order(doc());
        const auto             from  = std::find(order.begin(),
                                                 order.end(),
                                                 doc().ui.anchor);
        const auto             to    = std::find(order.begin(),
                                                 order.end(),
                                                 _id);

        if ( (from != order.end()) &&
             (to != order.end()) )
        {
            selection.assign((from < to) ? from : to,
                             ((from < to) ? to : from) + 1);
            ++doc().version;

            return;
        }
    }

    if (io.KeyCtrl)
    {
        const auto found = std::find(selection.begin(), selection.end(), _id);

        if (found != selection.end())
        {
            selection.erase(found);
        }
        else
        {
            selection.insert(selection.begin(), _id);
        }
    }
    else
    {
        selection.assign(1, _id);
    }

    doc().ui.anchor     = _id;
    doc().preview_valid = false;
    ++doc().version;

    return;
}

// ------------------------------------------------------------- rows: tree

/*
add_row
  A new row goes into the selected group, or beside the selected symbol,
or at the end of the sheet. A bit takes the lowest bit its group leaves
free; a range starts there, one bit wide; a value is placed in the range
selected, or the nearest range before it.
*/
void
app::add_row(
    int _kind
)
{
    const row* selected = inspected(doc());
    const int  parent   = (selected != nullptr) ? selected->id : 0;
    row        made     = new_row(m_shell.prefs);

    if (_kind < 0)
    {
        made.kind = row_kind::group;
        made.name = unique_name(doc().doc, "Group");
    }
    else
    {
        made.def = static_cast<def_kind>(std::clamp(_kind,
                                                    0,
                                                    DEF_KIND_COUNT - 1));

        const int free = next_free_bit(doc().doc, parent);

        switch (made.def)
        {
            case def_kind::bit:
                made.name = unique_name(doc().doc, "FLAG");
                made.bit  = (free < 0) ? 0 : free;
                break;

            case def_kind::range:
                made.name  = unique_name(doc().doc, "FIELD");
                made.name_children = true;
                made.lsb   = (free < 0) ? 0 : free;
                made.width = 1;
                break;

            case def_kind::field_value:
            {
                made.name = unique_name(doc().doc, "VALUE");

                const std::vector<int> ranges = range_choices(doc().doc);

                // the selected range, else the last range in the sheet
                if ( (selected != nullptr) &&
                     (selected->kind == row_kind::symbol) &&
                     (selected->def == def_kind::range) )
                {
                    made.field = selected->id;
                }
                else if (!ranges.empty())
                {
                    made.field = ranges.back();
                }

                break;
            }

            case def_kind::literal:
                made.name = unique_name(doc().doc, "VALUE");
                break;

            case def_kind::expression:
                made.name = unique_name(doc().doc, "COMBINED");
                made.expr = "0";
                break;

            case def_kind::macro:
                made.name   = unique_name(doc().doc, "MACRO");
                made.params = "x";
                made.body   = "(x)";
                break;
        }
    }

    int added = 0;

    edit(std::string(),
         [&added, parent, &made](sheet& _sheet)
         {
             added = insert_row(_sheet, parent, -1, made);
         });

    if (added != 0)
    {
        doc().ui.selection.assign(1, added);
        doc().ui.anchor = added;
        ++doc().version;
    }

    return;
}

/*
add_child
  A name entered in the "Add child..." field under a group's or a range's
rows is a bit of that name, at the lowest bit there that is free -- or, in
a range with none free, at the range's own lowest bit, where the collision
shows. The name is tried on a copy of the sheet first: one that is not a C
identifier once its affixes are added, or that is another symbol's, is
refused as a rename would be, and the field gets it back to be put right.
Either way the field has the keyboard again, for the next name.
*/
void
app::add_child(
    int                _parent,
    const std::string& _text
)
{
    const std::size_t first = _text.find_first_not_of(" \t\r\n");
    const std::size_t last  = _text.find_last_not_of(" \t\r\n");

    // nothing was typed
    if (first == std::string::npos)
    {
        return;
    }

    const std::string name   = _text.substr(first, last - first + 1);
    const row*        holder = find_row(doc().doc, _parent);

    // the field's group or range has gone, or holds rows no more
    if ( (holder == nullptr) ||
         (!can_contain(*holder)) )
    {
        status("there is no group or range to add " + name + " to", true);

        return;
    }

    const int free = next_free_bit(doc().doc, _parent);
    row       made = new_row(m_shell.prefs);

    made.def  = def_kind::bit;
    made.name = name;
    made.bit  = (free >= 0)                          ? free
              : (holder->kind == row_kind::symbol) ? holder->lsb
                                                   : 0;

    sheet               trial = doc().doc;
    const int           added = insert_row(trial, _parent, -1, made);
    const resolution    named = resolve(trial);
    const resolved_row* found = named.find(added);
    std::string         refused;

    if (found == nullptr)
    {
        refused = "there is no group or range to add " + name + " to";
    }
    else if (!is_identifier(found->full))
    {
        refused = "'" + found->full + "' is not a C identifier";
    }
    else
    {
        for (const resolved_row& each : named.rows)
        {
            if ( (each.id != added)                 &&
                 (each.kind == row_kind::symbol)    &&
                 (each.full == found->full) )
            {
                refused = found->full + " is already another symbol's name";

                break;
            }
        }
    }

    // refused: the field has the name back, and the status line says why
    if (!refused.empty())
    {
        doc().ui.adding_to    = _parent;
        doc().ui.adding_text  = _text;
        doc().ui.adding_focus = true;
        ++doc().version;
        status(refused, true);

        return;
    }

    const std::string said = found->full + " added, at bit " +
                             format_dec(static_cast<u64>(made.bit));

    edit(std::string(),
         [&trial](sheet& _sheet)
         {
             _sheet = std::move(trial);
         });

    doc().ui.selection.assign(1, added);
    doc().ui.anchor       = added;
    doc().ui.adding_to    = _parent;
    doc().ui.adding_text.clear();
    doc().ui.adding_focus = true;
    ++doc().version;
    status(said, false);

    return;
}

void
app::delete_rows()
{
    const std::vector<int> doomed = doc().ui.selection;

    if (doomed.empty())
    {
        return;
    }

    edit(std::string(),
         [&doomed](sheet& _sheet)
         {
             for (int id : doomed)
             {
                 (void)remove_row(_sheet, id);
             }

             // the display rules over them alone go with them, and the
             // special rows among them stay, where the rows were
             (void)prune_rules(_sheet);
             (void)settle_specials(_sheet);
         });

    doc().ui.selection.clear();
    status(format_dec(doomed.size()) + " row(s) deleted", false);

    return;
}

void
app::duplicate_rows()
{
    const std::vector<int> originals = doc().ui.selection;
    std::vector<int>       copies;

    if (originals.empty())
    {
        return;
    }

    edit(std::string(),
         [&originals, &copies](sheet& _sheet)
         {
             for (int id : originals)
             {
                 const row* found = find_row(_sheet, id);

                 if (found == nullptr)
                 {
                     continue;
                 }

                 row copy = *found;
                 copy.name = unique_name(_sheet, found->name + "_COPY");
                 copies.push_back(insert_row(_sheet, id, -1, copy));
             }
         });

    doc().ui.selection = copies;
    ++doc().version;

    return;
}

void
app::move_row_by(
    int _step
)
{
    const row* selected = inspected(doc());

    if (selected == nullptr)
    {
        return;
    }

    const int id = selected->id;

    edit(std::string(),
         [id, _step](sheet& _sheet)
         {
             row* parent = parent_of(_sheet, id);

             if (parent == nullptr)
             {
                 return;
             }

             for (std::size_t i = 0; i < parent->children.size(); ++i)
             {
                 if (parent->children[i].id != id)
                 {
                     continue;
                 }

                 const long target = static_cast<long>(i) + _step;

                 if ( (target >= 0) &&
                      (target < static_cast<long>(parent->children.size())) )
                 {
                     std::swap(parent->children[i],
                               parent->children[static_cast<std::size_t>(
                                   target)]);
                 }

                 return;
             }
         });

    sorted_note();

    return;
}

/*
sorted_note
  While the rows are shown sorted, a row moved by hand moves in the sheet
-- and the header -- but is shown where the sort puts it: the status line
says so.
*/
void
app::sorted_note()
{
    if (doc().doc.grid.sort_by != sort_key::sheet)
    {
        status("moved in the sheet's own order; the rows are shown sorted -- "
               "Make it the sheet's own order, on the Sort menu, to order "
               "them by hand",
               false);
    }

    return;
}

/*
drop_row
  A row dropped on a group goes to the end of it; dropped on a symbol, it
goes just before that symbol, in that symbol's group.
*/
void
app::drop_row(
    int _dragged,
    int _onto
)
{
    const row* onto = find_row(doc().doc, _onto);

    if ( (onto == nullptr) ||
         (_dragged == _onto) )
    {
        return;
    }

    int parent = _onto;
    int index  = -1;

    if (!can_contain(*onto))
    {
        const row* holder = parent_of(doc().doc, _onto);

        if (holder == nullptr)
        {
            return;
        }

        parent = holder->id;

        for (std::size_t i = 0; i < holder->children.size(); ++i)
        {
            if (holder->children[i].id == _onto)
            {
                index = static_cast<int>(i);
            }
        }
    }

    bool moved = false;

    edit(std::string(),
         [&moved, _dragged, parent, index](sheet& _sheet)
         {
             moved = move_row(_sheet, _dragged, parent, index);
         });

    if (!moved)
    {
        status("a group cannot be moved into itself", true);
    }
    else
    {
        sorted_note();
    }

    return;
}

// --------------------------------------------------------------- rows: edit

namespace
{

/*
write_field
  One of a row's fields, as the inspector edits it -- or the Add node
window, its row in the making: by its name, from the event's payload. A
range or a byte is worked out in the sheet the row is in, and a value's
default bias from its field's width there.
*/
void
write_field(
    row&               _row,
    const std::string& _field,
    const option_set&  _payload,
    const sheet&       _sheet,
    const resolution&  _res
)
{
    row&                   r      = _row;
    const std::string&     field  = _field;
    const long             number = _payload.as_long("value", 0);
    const bool             flag   = _payload.as_bool("value", false);
    const std::string      text   = _payload.as_string("value", "");
    const std::vector<int> ranges = range_choices(_sheet);

    if      (field == "name")       { r.name       = text; }
    else if (field == "short")      { r.short_name = text; }
    else if (field == "note")       { r.note       = text; }
    else if (field == "prefix")     { r.prefix     = text; }
    else if (field == "suffix")     { r.suffix     = text; }
    else if (field == "expr")       { r.expr       = text; }
    else if (field == "params")     { r.params     = text; }
    else if (field == "body")       { r.body       = text; }
    else if (field == "emit")       { r.emit       = flag; }
    else if (field == "collapsed")  { r.collapsed  = flag; }
    else if (field == "isolate")    { r.isolate    = flag; }
    else if (field == "name_children") { r.name_children = flag; }
    else if (field == "exclusive")  { r.exclusive  = flag; }
    else if (field == "negative")   { r.negative   = flag; }
    else if (field == "callable")   { r.callable   = flag; }
    else if (field == "digits")
    {
        r.digits = static_cast<int>(std::clamp<long>(number, 0, 64));
    }
    else if (field == "auto_colour")
    {
        r.colour = 0;
        r.transparent = false;
    }
    else if (field == "transparent")
    {
        r.transparent = flag;
    }
    else if (field == "bit")
    {
        r.bit = static_cast<int>(number);
    }
    else if (field == "lsb")
    {
        r.lsb = static_cast<int>(number);
    }
    else if (field == "width")
    {
        r.width = static_cast<int>(number);
    }
    else if (field == "msb")
    {
        r.width = static_cast<int>(number) - r.lsb + 1;
    }
    else if ( (field == "byte_low") ||
              (field == "byte_high") )
    {
        int low  = 0;
        int high = 0;
        int keep = 0;
        int top  = 0;

        byte_bits(_sheet, static_cast<int>(number), low, high);

        // the byte named and the end of the range not named
        if (field == "byte_low")
        {
            keep = r.lsb + r.width - 1;
            top  = std::max(keep, high);
            r.lsb   = std::min(low, keep);
            r.width = top - r.lsb + 1;
        }
        else
        {
            keep    = r.lsb;
            r.lsb   = std::min(keep, low);
            r.width = std::max(high, keep) - r.lsb + 1;
        }
    }
    else if (field == "def")
    {
        r.def = static_cast<def_kind>(
            std::clamp<long>(number, 0, DEF_KIND_COUNT - 1));

        // a value with nowhere to go takes the last range
        if ( (r.def == def_kind::field_value) &&
             (r.field == 0)                   &&
             (!ranges.empty()) )
        {
            r.field = ranges.back();
        }
    }
    else if (field == "field")
    {
        // choice 0 is "its own bits"; the rest are the ranges
        r.field = ( (number > 0) &&
                    (static_cast<std::size_t>(number) <=
                     ranges.size()) )
                      ? ranges[static_cast<std::size_t>(number) - 1]
                      : 0;
    }
    else if (field == "encoding")
    {
        r.enc = static_cast<encoding>(
            std::clamp<long>(number, 0, ENCODING_COUNT - 1));

        if ( (r.enc == encoding::offset_binary) &&
             (r.bias == 0) )
        {
            const resolved_row* field_row = _res.find(r.field);
            const int           width     =
                (field_row != nullptr) ? field_row->span_width
                                       : r.width;

            r.bias = default_bias(width);
        }
    }
    else if (field == "bias")
    {
        r.bias = number;
    }
    else if (field == "number")
    {
        const parsed_integer parsed = parse_integer(text);

        if (parsed.ok)
        {
            r.number = static_cast<i64>(parsed.value);
        }
    }
    else if (field == "literal")
    {
        const parsed_wide parsed = parse_wide(text);

        if (parsed.ok)
        {
            r.literal  = parsed.value;
            r.negative = parsed.negative;
        }
    }
    else if (field == "radix")
    {
        r.fmt = static_cast<radix>(
            std::clamp<long>(number, 0, 5) - 1);
    }
    else if (field == "int_suffix")
    {
        r.suf = static_cast<int_suffix>(
            std::clamp<long>(number, 0, 4) - 1);
    }
    else if (field == "colour")
    {
        r.transparent = false;
        r.colour = make_rgba(
            static_cast<float>(_payload.as_double("r", 1.0)),
            static_cast<float>(_payload.as_double("g", 1.0)),
            static_cast<float>(_payload.as_double("b", 1.0)),
            static_cast<float>(_payload.as_double("a", 1.0)));
    }

    return;
}

}  // anonymous namespace

void
app::row_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;
    const int          id      =
        static_cast<int>(payload.as_long("target", 0));
    const std::string  field   = kind.substr(4);

    if (find_row(doc().doc, id) == nullptr)
    {
        return;
    }

    const resolution& res = doc().res;

    edit("row." + field + ":" + format_dec(static_cast<u64>(id)),
         [&](sheet& _sheet)
         {
             write_field(*find_row(_sheet, id), field, payload, _sheet, res);
         });

    return;
}

// --------------------------------------------------------------- rows: grid

/*
grid_toggle
  What a click on a cell means depends on the row: a literal flips the bit;
a bit symbol moves to it; a value flips the bit within its field and is
read back as a number; a range stretches to take the bit in (a drag sets a
range exactly). Expressions and macros are defined by their text, and a
click leaves them be.
*/
void
app::grid_toggle(
    int _id,
    int _bit
)
{
    const row*          source   = find_row(doc().doc, _id);
    const resolved_row* resolved = doc().res.find(_id);

    if ( (source == nullptr)                  ||
         (resolved == nullptr)                ||
         (source->kind != row_kind::symbol)   ||
         (_bit < 0)                           ||
         (_bit >= doc().doc.width) )
    {
        return;
    }

    if ( (source->def == def_kind::expression) ||
         (source->def == def_kind::macro) )
    {
        status(source->name + " is defined by its text; edit it in the "
               "inspector", false);

        return;
    }

    if ( (source->def == def_kind::field_value) &&
         ( (_bit < resolved->span_lsb) ||
           (_bit >= resolved->span_lsb + resolved->span_width) ) )
    {
        status(source->name + "'s bits are its field's", false);

        return;
    }

    const int  lsb   = resolved->span_lsb;
    const int  width = resolved->span_width;

    edit("grid:" + format_dec(static_cast<u64>(_id)),
         [_id, _bit, lsb, width, resolved](sheet& _sheet)
         {
             row& r = *find_row(_sheet, _id);

             switch (r.def)
             {
                 case def_kind::bit:
                     r.bit = _bit;
                     break;

                 case def_kind::range:
                 {
                     // a click outside the range stretches it to the bit
                     const int high = std::max(r.lsb + r.width - 1, _bit);

                     r.lsb   = std::min(r.lsb, _bit);
                     r.width = high - r.lsb + 1;
                     break;
                 }

                 case def_kind::literal:
                     // the bits as they are shown, one flipped: a
                     // negative number becomes the bits it showed
                     r.literal  = resolved->bits;
                     r.literal.set(_bit, !resolved->bits.test(_bit));
                     r.negative = false;
                     break;

                 case def_kind::field_value:
                 {
                     const u64 bits = resolved->bits.field(lsb, width) ^
                                      range_mask(_bit - lsb, 1);

                     r.number = decode(r.enc, width, r.bias, bits);
                     break;
                 }

                 default:
                     break;
             }
         });

    return;
}

void
app::grid_span(
    int  _id,
    int  _low,
    int  _high,
    bool _set
)
{
    const row* source = find_row(doc().doc, _id);
    const int  low    = std::min(_low, _high);
    const int  high   = std::max(_low, _high);

    if ( (source == nullptr) ||
         (low < 0)           ||
         (high >= doc().doc.width) )
    {
        return;
    }

    const resolved_row* resolved = doc().res.find(_id);
    const wide          shown    = (resolved != nullptr) ? resolved->bits
                                                         : wide();

    edit("grid:" + format_dec(static_cast<u64>(_id)),
         [_id, low, high, _set, &shown](sheet& _sheet)
         {
             row& r = *find_row(_sheet, _id);

             if (r.def == def_kind::range)
             {
                 r.lsb   = low;
                 r.width = high - low + 1;
             }
             else if (r.def == def_kind::literal)
             {
                 // the bits as they are shown, a run set or cleared
                 const wide span = wide::run(low, high - low + 1);

                 r.literal  = _set ? (shown | span)
                                   : (shown & complement(span, MAX_WIDTH));
                 r.negative = false;
             }
         });

    return;
}

/*
collapse_segment
  A user's segment collapses and expands in place. An automatic one -- a
byte, say -- has nothing to keep the state in, so clicking it makes it a
segment of the user's, collapsed, named for its bits.
*/
void
app::collapse_segment(
    const option_set& _payload
)
{
    const long index = _payload.as_long("value", -1);
    const int  lsb   = static_cast<int>(_payload.as_long("lsb", 0));
    const int  msb   = static_cast<int>(_payload.as_long("msb", 0));

    quietly([index, lsb, msb](sheet& _sheet)
            {
                if ( (index >= 0) &&
                     (static_cast<std::size_t>(index) <
                      _sheet.segments.size()) )
                {
                    column_segment& segment =
                        _sheet.segments[static_cast<std::size_t>(index)];
                    segment.collapsed = !segment.collapsed;

                    return;
                }

                column_segment made;
                made.name = format_dec(static_cast<u64>(msb)) + ":" +
                            format_dec(static_cast<u64>(lsb));
                made.lsb       = lsb;
                made.msb       = msb;
                made.collapsed = true;
                _sheet.segments.push_back(made);
            });

    return;
}

// ------------------------------------------------------------ the console

namespace
{

// QUIET
//   constant: what the window tells itself, not what the user did: the
// console lists none of it.
const char* const QUIET[] = { "window.focus", "window.arranged" };

// value_text
//   one of an action's values, as the console writes it: a text quoted,
// and cut short.
std::string
value_text(
    const ::djinterp::option_value& _value
)
{
    if (const bool* flag = std::get_if<bool>(&_value))
    {
        return *flag ? "true" : "false";
    }

    if (const long* whole = std::get_if<long>(&_value))
    {
        return std::to_string(*whole);
    }

    if (const double* real = std::get_if<double>(&_value))
    {
        char written[32];

        std::snprintf(written, sizeof(written), "%g", *real);

        return written;
    }

    const std::string& text = std::get<std::string>(_value);
    std::string        line = text.substr(0, 48);

    std::replace(line.begin(), line.end(), '\n', ' ');

    return "\"" + line + ( (text.size() > 48) ? "...\"" : "\"" );
}

}  // anonymous namespace

/*
note_action
  What the user did, kept for the console: the action's name, what it
carried -- but the document it came from, which the console names as the
sheet -- and when; not what the window does by itself, as a sheet arrives
or a window takes the keyboard. The same action again, on the same target, is one
action more times over: a drag, a slider, a name typed letter by letter.
Only so many are kept, the oldest let go first.
*/
void
app::note_action(
    const event& _event
)
{
    for (const char* each : QUIET)
    {
        if (_event.kind == each)
        {
            return;
        }
    }

    // the grid fitting its columns to a sheet just arrived
    if (_event.payload.as_bool("arriving", false))
    {
        return;
    }

    std::string detail;

    for (const auto& entry : _event.payload.entries)
    {
        if (entry.first == "document")
        {
            continue;
        }

        detail += (detail.empty() ? "" : ", ") + entry.first + " " +
                  value_text(entry.second);
    }

    const long         owner = _event.payload.as_long("document", 0);
    const model*       found = (owner != 0)
                                   ? document(static_cast<int>(owner))
                                   : m_doc;
    const std::string  path  = (found != nullptr) ? found->ui.path
                                                  : std::string();
    const std::string  sheet = path.substr(path.find_last_of("/\\") + 1);
    const std::time_t  now   = std::time(nullptr);
    const std::tm*     local = std::localtime(&now);
    char               when[16] = "";

    if (local != nullptr)
    {
        (void)std::strftime(when, sizeof(when), "%H:%M:%S", local);
    }

    std::vector<action_entry>& actions = m_shell.actions;
    const ::djinterp::option_value* target = _event.payload.find("target");

    // the same action again, on the same target: one more time
    if ( (!actions.empty())                      &&
         (actions.back().kind == _event.kind)    &&
         (actions.back().sheet == sheet)         &&
         ( (actions.back().detail == detail)  ||
           ( (target != nullptr) &&
             (detail.find("target " + value_text(*target)) !=
              std::string::npos) &&
             (actions.back().detail.find("target " + value_text(*target)) !=
              std::string::npos) ) ) )
    {
        ++actions.back().times;
        actions.back().detail = detail;
        actions.back().when   = when;
    }
    else
    {
        actions.push_back(action_entry{ when, _event.kind, detail, sheet, 1 });

        if (actions.size() > ACTIONS_KEPT)
        {
            actions.erase(actions.begin());
        }
    }

    return;
}

// ------------------------------------------------------------- Add node

/*
node_edit
  The Add node window: "node.open" opens it on the document the event is
from, its row going into group or range "value" -- 0 for the sheet's own
rows; "node.close" closes it. "node.kind" makes its row another kind, by
NODE_KINDS' index; "node.parent", by container_choices' index, puts it in
another group or range; "node.first", first there or last. Every other
"node.FIELD" is one of the row's fields, as the inspector's "row.FIELD"
is. "node.add" adds the row -- "close" closing the window after -- and the
window starts the next. Nothing but the add is a step to undo.
*/
void
app::node_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;
    const std::string  field   = kind.substr(5);
    node_draft&        n       = doc().ui.node;

    if (field == "open")
    {
        // into the group or range named, or the one the row selected is,
        // or is in
        int        into     = static_cast<int>(payload.as_long("value", 0));
        const row* selected = inspected(doc());

        if ( (!payload.has("value")) &&
             (selected != nullptr) )
        {
            const row* holder = can_contain(*selected)
                                    ? selected
                                    : parent_of(doc().doc, selected->id);

            into = (holder != nullptr) ? holder->id : 0;
        }

        open_node(into);

        return;
    }

    if (field == "close")
    {
        m_shell.node_open = false;
        shell_changed();

        return;
    }

    if (field == "add")
    {
        add_node(payload.as_bool("close", false));

        return;
    }

    if (field == "kind")
    {
        const long chosen = std::clamp<long>(payload.as_long("value", 0),
                                             0,
                                             NODE_KIND_COUNT - 1);
        row        made   = new_row(m_shell.prefs);

        // what the kind starts with; the name, notes and colour kept
        made.name       = n.draft.name;
        made.short_name = n.draft.short_name;
        made.note       = n.draft.note;
        made.colour     = n.draft.colour;
        made.transparent = n.draft.transparent;

        if (chosen == NODE_KIND_COUNT - 1)
        {
            made.kind = row_kind::group;
        }
        else
        {
            made.def = static_cast<def_kind>(chosen);

            if (made.def == def_kind::field_value)
            {
                const std::vector<int> ranges = range_choices(doc().doc);
                const row*             holder = find_row(doc().doc,
                                                         n.parent);

                made.field = ( (holder != nullptr)                    &&
                               (holder->kind == row_kind::symbol)     &&
                               (holder->def == def_kind::range) )
                                 ? holder->id
                                 : (ranges.empty() ? 0 : ranges.back());
            }
            else if (made.def == def_kind::expression)
            {
                made.expr = "0";
            }
            else if (made.def == def_kind::macro)
            {
                made.params = "x";
                made.body   = "(x)";
            }
        }

        n.draft    = made;
        n.bits_set = false;
    }
    else if (field == "parent")
    {
        const std::vector<int> choices = container_choices(doc().doc);
        const long             index   = payload.as_long("value", 0);

        n.parent   = ( (index >= 0) &&
                       (static_cast<std::size_t>(index) < choices.size()) )
                         ? choices[static_cast<std::size_t>(index)]
                         : 0;
        n.bits_set = false;
    }
    else if (field == "first")
    {
        n.first = (payload.as_long("value", 0) != 0);
    }
    else
    {
        write_field(n.draft, field, payload, doc().doc, doc().res);

        n.bits_set = ( (n.bits_set)              ||
                       (field == "bit")          ||
                       (field == "lsb")          ||
                       (field == "width")        ||
                       (field == "msb")          ||
                       (field == "byte_low")     ||
                       (field == "byte_high") );
    }

    refresh_node();
    ++doc().version;

    return;
}

/*
open_node
  The Add node window, open on the document in front, its row going into a
group or range -- a bit, at the next bit free there, until another kind is
chosen; one already being made keeps what it has, in its new place. It is
brought to the front.
*/
void
app::open_node(
    int _parent
)
{
    node_draft& n      = doc().ui.node;
    const row*  holder = find_row(doc().doc, _parent);

    if ( (n.fresh.empty()) &&
         (n.draft.name.empty()) )
    {
        n.draft          = new_row(m_shell.prefs);
        n.draft.def      = def_kind::bit;
    }

    n.parent   = ( (holder != nullptr) &&
                   (can_contain(*holder)) ) ? _parent : 0;
    n.bits_set = false;

    m_shell.node_open  = true;
    m_shell.node_raise = true;
    refresh_node();
    ++doc().version;
    shell_changed();

    return;
}

/*
refresh_node
  What the row in the making comes to: put in a copy of the sheet where it
would go -- at the next bit free there, unless its bits were chosen -- and
the copy resolved. A name that is not a C identifier once its affixes are
added, or that is another symbol's, is why it would be refused; the issues
the copy finds with it are shown with it.
*/
void
app::refresh_node()
{
    node_draft& n = doc().ui.node;

    // its group gone: the sheet's own rows
    if (n.parent != 0)
    {
        const row* holder = find_row(doc().doc, n.parent);

        if ( (holder == nullptr) ||
             (!can_contain(*holder)) )
        {
            n.parent = 0;
        }
    }

    // its bits: the next free, until it is given its own
    if ( (!n.bits_set) &&
         (n.draft.kind == row_kind::symbol) )
    {
        const row* holder = find_row(doc().doc, n.parent);
        const int  free   = next_free_bit(doc().doc, n.parent);
        const int  at     = (free >= 0) ? free
                          : ( (holder != nullptr) &&
                              (holder->kind == row_kind::symbol) )
                                ? holder->lsb
                                : 0;

        n.draft.bit = at;
        n.draft.lsb = at;
    }

    // the name it takes when none is typed
    static const char* const BASES[] = { "FLAG", "FIELD", "VALUE", "VALUE",
                                         "COMBINED", "MACRO" };

    n.fresh = unique_name(doc().doc,
                          (n.draft.kind == row_kind::group)
                              ? std::string("Group")
                              : std::string(BASES[std::clamp(
                                    static_cast<int>(n.draft.def), 0, 5)]));

    // tried on a copy
    row made = n.draft;

    if (made.name.empty())
    {
        made.name = n.fresh;
    }

    sheet               trial = doc().doc;
    const int           added = insert_row(trial, n.parent,
                                           n.first ? 0 : -1, made);
    const resolution    named = resolve(trial, doc().shown.colours);
    const resolved_row* found = named.find(added);

    n.resolves = (found != nullptr);
    n.issues.clear();
    n.refused.clear();

    if (found == nullptr)
    {
        n.refused = "there is no group or range to add it to";

        return;
    }

    n.resolved = *found;

    for (const issue& each : named.issues)
    {
        if (each.row == added)
        {
            n.issues.push_back(each);
        }
    }

    if (found->kind != row_kind::symbol)
    {
        return;
    }

    if (!is_identifier(found->full))
    {
        n.refused = "'" + found->full + "' is not a C identifier";

        return;
    }

    for (const resolved_row& each : named.rows)
    {
        if ( (each.id != added)               &&
             (each.kind == row_kind::symbol)  &&
             (each.full == found->full) )
        {
            n.refused = found->full + " is already another symbol's name";

            break;
        }
    }

    return;
}

/*
add_node
  The row in the making, added where it goes: one step to undo, selected,
and said on the status line. The window starts the next -- the same kind,
in the same place, at the next bit free, with no name typed -- or closes.
A row it would refuse is not added; the status line says why.
*/
void
app::add_node(
    bool _close
)
{
    node_draft& n = doc().ui.node;

    refresh_node();

    if (!n.refused.empty())
    {
        status(n.refused, true);
        ++doc().version;

        return;
    }

    row made = n.draft;

    if (made.name.empty())
    {
        made.name = n.fresh;
    }

    const int   parent = n.parent;
    const bool  first  = n.first;
    int         added  = 0;

    edit(std::string(),
         [&added, &made, parent, first](sheet& _sheet)
         {
             added = insert_row(_sheet, parent, first ? 0 : -1, made);
         });

    const resolved_row* found = doc().res.find(added);
    std::string         said  =
        ( (found != nullptr) && (!found->full.empty()) ) ? found->full
                                                         : made.name;

    said += " added";

    if ( (made.kind == row_kind::symbol) &&
         (made.def == def_kind::bit) )
    {
        said += ", at bit " + format_dec(static_cast<u64>(made.bit));
    }

    if (added != 0)
    {
        doc().ui.selection.assign(1, added);
        doc().ui.anchor = added;
        ++n.added;
    }

    status(said, false);

    // the next: no name typed, at the next bit free
    n.draft.name.clear();
    n.draft.short_name.clear();
    n.draft.note.clear();
    n.bits_set = false;
    refresh_node();
    ++doc().version;

    if (_close)
    {
        m_shell.node_open = false;
        shell_changed();
    }

    return;
}

// ------------------------------------------------------- the special rows

/*
special_edit
  The probe and usage rows: where each is shown, and how many. Each names a
special row by its id, "value". "special.place" puts it "place" -- "top",
"bottom" or "tree" -- and, among the rows, before row "before", or at the
end of group "parent"'s rows; "ahead" is the special row it is put before
in their order, 0 for last. "special.duplicate" adds another of its kind
just after it, placed as it is; "special.remove" takes it away -- the last
of its kind is hidden instead, and keeps its place. Each shows its kind,
and is one step to undo.
*/
void
app::special_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;
    const int          id      = static_cast<int>(payload.as_long("value", 0));
    const special_row* found   = find_special(doc().doc.grid, id);

    if (found == nullptr)
    {
        return;
    }

    const special_kind of   = found->kind;
    const char*        what = (of == special_kind::probe) ? "probe"
                                                          : "usage";

    // shown
    //   the grid's switch for every row of the kind.
    auto shown = [of](grid_settings& _grid) -> bool&
    {
        return (of == special_kind::probe) ? _grid.show_probe
                                           : _grid.show_usage;
    };

    if (kind == "special.place")
    {
        const std::string where  = payload.as_string("place", "bottom");
        const special_place place =
            (where == "top")  ? special_place::top
          : (where == "tree") ? special_place::tree
                              : special_place::bottom;
        const int parent = static_cast<int>(payload.as_long("parent", 0));
        const int before = static_cast<int>(payload.as_long("before", 0));
        const int ahead  = static_cast<int>(payload.as_long("ahead", 0));

        edit(std::string(),
             [&](sheet& _sheet)
             {
                 (void)place_special(_sheet.grid, id, place, parent, before,
                                     ahead);
                 shown(_sheet.grid) = true;
                 (void)settle_specials(_sheet);
             });

        status(std::string("the ") + what + " row " +
                   ( (place == special_place::top)    ? "pinned to the top"
                   : (place == special_place::bottom) ? "pinned to the bottom"
                   : (of == special_kind::usage)
                         ? "placed among the rows: in a group, it counts "
                           "the group's"
                         : "placed among the rows" ),
               false);
    }
    else if (kind == "special.duplicate")
    {
        edit(std::string(),
             [&](sheet& _sheet)
             {
                 (void)add_special(_sheet.grid, of, id);
                 shown(_sheet.grid) = true;
             });

        int count = 0;

        for (const special_row& each : doc().doc.grid.specials)
        {
            count += (each.kind == of) ? 1 : 0;
        }

        status(std::string("another ") + what + " row: " +
                   format_dec(static_cast<u64>(count)) +
                   ( (of == special_kind::probe) ? ", all showing the one "
                                                   "probe"
                                                 : " now" ),
               false);
    }
    else if (kind == "special.remove")
    {
        edit(std::string(),
             [&](sheet& _sheet)
             {
                 (void)remove_special(_sheet.grid, id);
             });

        status(std::string("the ") + what + " row removed", false);
    }

    return;
}

// ------------------------------------------------------- the display rules

/*
display_edit
  What a part of the grid shows. "display.set" shows or hides a part --
digits, names, colours -- over a region: rows, by id, as "rows" lists
them ("" for every row), from bit "lsb" to "msb" (every bit when msb is
below lsb), "value" 1 shown, 0 hidden, -1 as the rest of the grid; "only"
there and nowhere else. "display.reset" gives a region back to the grid:
every part. "display.edit" sets one part of one rule, "target" "N.P", from
the Display window's combo: 0 as the grid, 1 shown, 2 hidden.
"display.remove" removes rule "value"; "display.clear", every rule. Each is
one step to undo.
*/
void
app::display_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;

    if ( (kind == "display.set") ||
         (kind == "display.reset") )
    {
        std::vector<int>  rows;
        const std::string text = payload.as_string("rows", "");
        std::size_t       at   = 0;

        while (at < text.size())
        {
            const std::size_t comma = text.find(',', at);
            const std::string each  = text.substr(at, comma - at);

            if (!each.empty())
            {
                rows.push_back(static_cast<int>(parse_integer(each).value));
            }

            at = (comma == std::string::npos) ? text.size() : comma + 1;
        }

        const int  lsb   = static_cast<int>(payload.as_long("lsb", 0));
        const int  msb   = static_cast<int>(payload.as_long("msb", -1));
        const int  part  = static_cast<int>(
            std::clamp(payload.as_long("part", 0),
                       0L,
                       static_cast<long>(SHOWN_PART_COUNT - 1)));
        const int  value = static_cast<int>(payload.as_long("value", 1));
        const bool only  = payload.as_bool("only", false);
        const bool reset = (kind == "display.reset");

        edit(std::string(),
             [&](sheet& _sheet)
             {
                 if (reset)
                 {
                     for (int i = 0; i < SHOWN_PART_COUNT; ++i)
                     {
                         set_shown(_sheet.grid, rows, lsb, msb,
                                   static_cast<shown_part>(i), -1, false);
                     }

                     return;
                 }

                 set_shown(_sheet.grid, rows, lsb, msb,
                           static_cast<shown_part>(part), value, only);
             });

        return;
    }

    if (kind == "display.remove")
    {
        const long index = payload.as_long("value", -1);

        edit(std::string(),
             [index](sheet& _sheet)
             {
                 std::vector<display_rule>& rules = _sheet.grid.rules;

                 if ( (index >= 0) &&
                      (static_cast<std::size_t>(index) < rules.size()) )
                 {
                     rules.erase(rules.begin() + index);
                 }
             });

        return;
    }

    if (kind == "display.clear")
    {
        edit(std::string(),
             [](sheet& _sheet)
             {
                 _sheet.grid.rules.clear();
             });

        return;
    }

    if (kind != "display.edit")
    {
        return;
    }

    // display.edit: target "rule.part", value the combo's choice
    const std::string target = payload.as_string("target", "");
    const std::size_t dot    = target.find('.');

    if (dot == std::string::npos)
    {
        return;
    }

    const long index  = static_cast<long>(
        parse_integer(target.substr(0, dot)).value);
    const long part   = static_cast<long>(
        parse_integer(target.substr(dot + 1)).value);
    const long choice = payload.as_long("value", 0);
    const int  value  = (choice == 1) ? 1 : ( (choice == 2) ? 0 : -1 );

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= doc().doc.grid.rules.size()) ||
         (part < 0) ||
         (part >= SHOWN_PART_COUNT) )
    {
        return;
    }

    edit("display." + target,
         [index, part, value](sheet& _sheet)
         {
             std::vector<display_rule>& rules = _sheet.grid.rules;

             rule_part(rules[static_cast<std::size_t>(index)],
                       static_cast<shown_part>(part)) = value;

             // a rule left setting nothing goes
             const display_rule& each = rules[static_cast<std::size_t>(index)];

             if ( (each.digits < 0) &&
                  (each.names < 0)  &&
                  (each.colour < 0) )
             {
                 rules.erase(rules.begin() + index);
             }
         });

    return;
}

/*
keep_sorted
  The order the rows are shown in, made the sheet's own: each group's rows
put in the order they are shown, and the sort put back to the sheet's --
so the header, which follows the sheet, follows it too. One step to undo.
*/
void
app::keep_sorted()
{
    if (doc().doc.grid.sort_by == sort_key::sheet)
    {
        return;
    }

    // each row's place among those shown, by its id
    std::map<int, std::size_t> place;

    for (const resolved_row& each : doc().res.rows)
    {
        place[each.id] = place_of(doc(), each.id);
    }

    // in_order
    //   a group's rows, and those of every group in it, put in place.
    std::function<void(row&)> in_order = [&](row& _row)
    {
        std::stable_sort(_row.children.begin(),
                         _row.children.end(),
                         [&place](const row& _a,
                                  const row& _b)
                         {
                             const auto a = place.find(_a.id);
                             const auto b = place.find(_b.id);
                             const std::size_t at_a =
                                 (a == place.end()) ? place.size() : a->second;
                             const std::size_t at_b =
                                 (b == place.end()) ? place.size() : b->second;

                             return (at_a < at_b);
                         });

        for (row& child : _row.children)
        {
            in_order(child);
        }
    };

    edit(std::string(),
         [&in_order](sheet& _sheet)
         {
             in_order(_sheet.root);
             _sheet.grid.sort_by = sort_key::sheet;
         });

    status("the rows are in the sheet in the order they were shown", false);

    return;
}

// ------------------------------------------------- segments and templates

void
app::segment_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;

    if (kind == "segment.add")
    {
        const row* selected = inspected(doc());
        int        lsb      = 0;
        int        msb      = std::min(7, doc().doc.width - 1);

        // a selected range gives the new segment its bits
        if ( (selected != nullptr) &&
             (selected->kind == row_kind::symbol) &&
             (selected->def == def_kind::range) )
        {
            lsb = selected->lsb;
            msb = selected->lsb + selected->width - 1;
        }

        edit(std::string(),
             [lsb, msb, selected](sheet& _sheet)
             {
                 column_segment made;
                 made.name = (selected != nullptr) ? selected->name
                                                   : std::string("segment");
                 made.lsb  = lsb;
                 made.msb  = msb;
                 _sheet.segments.push_back(made);
             });

        return;
    }

    if (kind == "segment.remove")
    {
        const long index = payload.as_long("value", -1);

        edit(std::string(),
             [index](sheet& _sheet)
             {
                 if ( (index >= 0) &&
                      (static_cast<std::size_t>(index) <
                       _sheet.segments.size()) )
                 {
                     _sheet.segments.erase(_sheet.segments.begin() + index);
                 }
             });

        return;
    }

    // segment.set: target "index.field"
    const std::string target = payload.as_string("target", "");
    const std::size_t dot    = target.find('.');

    if (dot == std::string::npos)
    {
        return;
    }

    const std::size_t index = static_cast<std::size_t>(
        parse_integer(target.substr(0, dot)).value);
    const std::string field = target.substr(dot + 1);

    edit("segment:" + target,
         [index, field, &payload](sheet& _sheet)
         {
             if (index >= _sheet.segments.size())
             {
                 return;
             }

             column_segment& segment = _sheet.segments[index];

             if (field == "name")
             {
                 segment.name = payload.as_string("value", segment.name);
             }
             else if (field == "lsb")
             {
                 segment.lsb = static_cast<int>(
                     payload.as_long("value", segment.lsb));
             }
             else if (field == "msb")
             {
                 segment.msb = static_cast<int>(
                     payload.as_long("value", segment.msb));
             }
             else if (field == "collapsed")
             {
                 segment.collapsed = payload.as_bool("value",
                                                     segment.collapsed);
             }
             else if (field == "colour")
             {
                 segment.colour = make_rgba(
                     static_cast<float>(payload.as_double("r", 1.0)),
                     static_cast<float>(payload.as_double("g", 1.0)),
                     static_cast<float>(payload.as_double("b", 1.0)),
                     static_cast<float>(payload.as_double("a", 1.0)));
             }
         });

    return;
}

void
app::template_edit(
    const event& _event
)
{
    const std::string& kind    = _event.kind;
    const option_set&  payload = _event.payload;

    if (kind == "template.add")
    {
        edit(std::string(),
             [](sheet& _sheet)
             {
                 macro_template made;
                 made.applies = applies_bit(def_kind::range);
                 made.name    = "{NAME}_MACRO";

                 // a wide sheet's, over an array
                 if (is_wide(_sheet.width))
                 {
                     made.wide   = true;
                     made.params = "a";
                     made.body   = "({GET})";
                     made.note   = "{LOCAL}, read from the array a";
                 }
                 else
                 {
                     made.params = "x";
                     made.body   = "((x) & {NAME})";
                     made.note   = "x masked by {LOCAL}";
                 }

                 _sheet.gen.templates.push_back(made);
             });

        return;
    }

    if (kind == "template.remove")
    {
        const long index = payload.as_long("value", -1);

        edit(std::string(),
             [index](sheet& _sheet)
             {
                 if ( (index >= 0) &&
                      (static_cast<std::size_t>(index) <
                       _sheet.gen.templates.size()) )
                 {
                     _sheet.gen.templates.erase(
                         _sheet.gen.templates.begin() + index);
                 }
             });

        return;
    }

    if (kind == "template.defaults")
    {
        edit(std::string(),
             [](sheet& _sheet)
             {
                 // the sheet's own kind; the other kind's stay as they are
                 const bool                  wide = is_wide(_sheet.width);
                 std::vector<macro_template> kept;

                 for (const macro_template& each : _sheet.gen.templates)
                 {
                     if (each.wide != wide)
                     {
                         kept.push_back(each);
                     }
                 }

                 for (const macro_template& each :
                      (wide ? default_wide_templates() : default_templates()))
                 {
                     kept.push_back(each);
                 }

                 _sheet.gen.templates = std::move(kept);
             });

        return;
    }

    // template.set: target "index.field"
    const std::string target = payload.as_string("target", "");
    const std::size_t dot    = target.find('.');

    if (dot == std::string::npos)
    {
        return;
    }

    const std::size_t index = static_cast<std::size_t>(
        parse_integer(target.substr(0, dot)).value);
    const std::string field = target.substr(dot + 1);

    edit("template:" + target,
         [index, field, &payload](sheet& _sheet)
         {
             if (index >= _sheet.gen.templates.size())
             {
                 return;
             }

             macro_template& made = _sheet.gen.templates[index];

             if (field == "enabled")
             {
                 made.enabled = payload.as_bool("value", made.enabled);
             }
             else if (field == "name")
             {
                 made.name = payload.as_string("value", made.name);
             }
             else if (field == "params")
             {
                 made.params = payload.as_string("value", made.params);
             }
             else if (field == "body")
             {
                 made.body = payload.as_string("value", made.body);
             }
             else if (field == "note")
             {
                 made.note = payload.as_string("value", made.note);
             }
             else if (field.compare(0, 8, "applies.") == 0)
             {
                 const int  bit =
                     1 << static_cast<int>(
                         parse_integer(field.substr(8)).value & 7u);
                 const bool on  = payload.as_bool("value", false);

                 made.applies = on ? (made.applies | bit)
                                   : (made.applies & ~bit);
             }
         });

    return;
}

// ------------------------------------------------------------------- files

/*
open_file
  A file opened in a document of its own -- or in a new sheet never
touched, or an example only looked at, which it takes the place of. A file
already open brings its document to the front instead. The path typed into
the document in front's field was a file to open, not that document's new
name: the field has the document's own file back.
*/
void
app::open_file(
    const std::string& _path
)
{
    if (_path.empty())
    {
        status("type the path of a sheet, a bitfield or a header in the "
               "field, then Open",
               true);

        return;
    }

    for (const std::unique_ptr<model>& each : m_docs)
    {
        if ( (each->ui.file == _path) &&
             (!each->ui.browsing) )
        {
            const int id = each->id;

            if (doc().ui.path != doc().ui.file)
            {
                doc().ui.path = doc().ui.file;
                ++doc().version;
            }

            activate(id);
            status(_path + " is open already", false);

            return;
        }
    }

    std::string text;
    std::string error;

    if (!read_file(_path, text, error))
    {
        status(error, true);

        return;
    }

    if ( (!doc().ui.pristine)        &&
         (!doc().ui.browsing)        &&
         (!doc().ui.file.empty())    &&
         (doc().ui.path != doc().ui.file) )
    {
        doc().ui.path = doc().ui.file;
        ++doc().version;
    }

    // a header, or any other code, is imported as a new sheet
    if (!looks_like_sheet(text))
    {
        import_code_text(_path, text);

        return;
    }

    import_json_text(_path, text);

    return;
}

/*
app::macro_header
  A deliberately minimal export. Each usable symbol gets one #define, with
  symbolic expression references rewritten to their full macro names.
*/
std::string
app::macro_header() const
{
    return generate_simple_defines(doc().doc, doc().res);
}

void
app::save_file(
    int _extension
)
{
    std::string path = doc().ui.path;
    std::string error;

    // the header's own extension, unless Save As chose one
    const std::string& file = doc().doc.gen.file_name;
    const int          kind =
        (_extension >= 0) ? _extension
                          : ( ( (file.size() >= 4) &&
                                (file.compare(file.size() - 4, 4, ".hpp") ==
                                 0) ) ? 1 : 0 );

    if (path.empty())
    {
        status("Choose a JSON document path first (File > Save As).", true);
        return;
    }

    std::string name, stem, extension;
    file_parts(path, name, stem, extension);
    if (extension != "json") path += ".bitwisedom.json";

    const std::string header_path =
        directory_of(path) + json_stem(path) +
        ((kind == 1) ? ".hpp" : ".h");

    // The companion's extension is part of the saved document settings.
    doc().doc.gen.file_name = json_stem(path) +
                              ((kind == 1) ? ".hpp" : ".h");

    // Only seal history and clear the dirty flag if BOTH files succeed.
    if (!write_file(path, save_sheet(doc().doc), error))
    {
        status(error, true);
        return;
    }
    if (!write_file(header_path, macro_header(), error))
    {
        status("JSON saved, but header failed: " + error, true);
        return;
    }

    doc().ui.path      = path;
    doc().ui.file      = path;
    doc().ui.dirty     = false;
    doc().ui.pristine  = false;
    doc().ui.browsing  = false;
    doc().header_valid = false;
    doc().steps.seal();
    status("saved " + path + " and " + header_path, false);
    ++doc().version;

    // its window's title says so
    shell_changed();
}

/*
rename_file
  The sheet's file named anew, in its tab: the folder it is in kept, and
".bitwisedom.json" put after a name with no ".json" of its own. A name
that is empty, or names a folder, or holds what no file name may, is
refused, and the tab keeps its name. Nothing on the disk changes: the sheet
is marked changed, and Save writes it under its new name.
*/
void
app::rename_file(
    const std::string& _text
)
{
    const std::size_t first = _text.find_first_not_of(" \t\r\n");
    const std::size_t last  = _text.find_last_not_of(" \t\r\n");

    doc().ui.file_renaming = false;
    ++doc().version;

    if (first == std::string::npos)
    {
        status("a file needs a name", true);

        return;
    }

    std::string name = _text.substr(first, last - first + 1);

    if ( (name.find_first_of("/\\<>:\"|?*") != std::string::npos) ||
         (name == ".") ||
         (name == "..") )
    {
        status("'" + name + "' is not a file's name: no folders, and none "
               "of < > : \" / \\ | ? *", true);

        return;
    }

    const std::string json = ".json";

    if ( (name.size() < json.size()) ||
         (name.compare(name.size() - json.size(), json.size(), json) != 0) )
    {
        const std::string sheet = ".bitwisedom";

        name += ( (name.size() >= sheet.size()) &&
                  (name.compare(name.size() - sheet.size(), sheet.size(),
                                sheet) == 0) )
                    ? json
                    : sheet + json;
    }

    const std::string& path   = doc().ui.path;
    const std::size_t  slash  = path.find_last_of("/\\");
    const std::string  folder = (slash == std::string::npos)
                                    ? std::string()
                                    : path.substr(0, slash + 1);
    const std::string  was    = (slash == std::string::npos)
                                    ? path
                                    : path.substr(slash + 1);

    if (name == was)
    {
        return;
    }

    doc().ui.path     = folder + name;
    doc().ui.dirty    = true;
    doc().ui.pristine = false;
    status("renamed " + name + ": Save writes it under that name", false);
    shell_changed();

    return;
}

/*
open_dialog
  File > Open's question: the file to open -- a sheet, a bitfield or a
header -- typed, or, on Windows, chosen in Explorer's picker. Enter or Open
opens it as a new sheet; Escape or Cancel asks nothing.
*/
void
app::open_dialog()
{
    if (m_open_open)
    {
        ImGui::OpenPopup("Open##bitwisedom");
        m_open_open = false;
    }

    if (!ImGui::BeginPopupModal("Open##bitwisedom", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextUnformatted("A sheet (.bitwisedom.json), a bitfield (.json), "
                           "or a C or C++ header");

    if (m_open_focus)
    {
        ImGui::SetKeyboardFocusHere();
        m_open_focus = false;
    }

    ImGui::SetNextItemWidth(480.0f);

    const bool entered = ImGui::InputText("##open_path", &m_open_path,
                                          ImGuiInputTextFlags_EnterReturnsTrue);

#if defined(_WIN32)
    ImGui::SameLine();

    if (ImGui::Button("Browse..."))
    {
        std::vector<wchar_t> name(32768, L'\0');
        OPENFILENAMEW        chooser{};

        chooser.lStructSize = sizeof(chooser);
        chooser.hwndOwner   = GetActiveWindow();
        chooser.lpstrFilter = L"Sheets, bitfields and headers\0"
                              L"*.json;*.h;*.hpp;*.hh;*.hxx\0"
                              L"All files\0*.*\0\0";
        chooser.lpstrFile   = name.data();
        chooser.nMaxFile    = static_cast<DWORD>(name.size());
        chooser.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST |
                              OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&chooser))
        {
            const std::wstring chosen(name.data());
            const int          size = WideCharToMultiByte(
                CP_UTF8, 0, chosen.c_str(), static_cast<int>(chosen.size()),
                nullptr, 0, nullptr, nullptr);
            std::string        out(static_cast<std::size_t>(std::max(size, 0)),
                                   '\0');

            if (size > 0)
            {
                WideCharToMultiByte(CP_UTF8, 0, chosen.c_str(),
                                    static_cast<int>(chosen.size()),
                                    &out[0], size, nullptr, nullptr);
            }

            m_open_path = out;
        }
    }
#endif

    ImGui::Separator();

    const bool valid =
        (m_open_path.find_first_not_of(" \t\r\n") != std::string::npos);

    ImGui::BeginDisabled(!valid);

    if ( (ImGui::Button("Open", ImVec2(110.0f, 0.0f))) ||
         ( (valid) && (entered) ) )
    {
        open_file(m_open_path);
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if ( (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f))) ||
         (ImGui::IsKeyPressed(ImGuiKey_Escape)) )
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return;
}

void
app::save_as_dialog()
{
    if (m_save_as_open)
    {
        ImGui::OpenPopup("Save As##bitwisedom");
        m_save_as_open = false;
    }

    if (ImGui::BeginPopupModal("Save As##bitwisedom", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Document (JSON)");
        if (m_save_as_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_save_as_focus = false;
        }
        ImGui::SetNextItemWidth(440.0f);
        ImGui::InputText("##save_path", &m_save_as_path);
        ImGui::TextUnformatted("Companion macro file");
        ImGui::RadioButton(".h", &m_save_as_extension, 0);
        ImGui::SameLine();
        ImGui::RadioButton(".hpp", &m_save_as_extension, 1);
        ImGui::Separator();

        const bool valid = !m_save_as_path.empty();
        ImGui::BeginDisabled(!valid);
        if (ImGui::Button("Save", ImVec2(110, 0)) ||
            (valid && ImGui::IsKeyPressed(ImGuiKey_Enter)))
        {
            doc().ui.path = m_save_as_path;
            save_file(m_save_as_extension);
            if (!doc().ui.dirty && !doc().ui.status_error)
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110, 0)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// Import multiple existing preprocessor sources into independent editor tabs.
// One file path per line: each source is embedded in the sheet's JSON.
void
app::import_definitions_dialog()
{
    if (m_import_open)
    {
        ImGui::OpenPopup("Import definition files##bitwisedom");
        m_import_open = false;
    }

    if (!ImGui::BeginPopupModal("Import definition files##bitwisedom",
                                nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextUnformatted("File paths (one .h/.hpp/.c/.cpp per line)");
    ImGui::SetNextItemWidth(560.0f);
    ImGui::InputTextMultiline("##definition.paths", &m_import_paths,
                              ImVec2(560.0f, 130.0f));
#if defined(_WIN32)
    // The Explorer file picker supports multi-selection. The list of
    // absolute UTF-8 paths is then shown in the field above.
    if (ImGui::Button("Browse files..."))
    {
        std::vector<wchar_t> names(32768, L'\0');
        OPENFILENAMEW chooser{};
        chooser.lStructSize = sizeof(chooser);
        chooser.hwndOwner = GetActiveWindow();
        chooser.lpstrFilter =
            L"C/C++ sources\0*.h;*.hpp;*.hh;*.hxx;*.c;*.cpp;*.inl\0"
            L"All files\0*.*\0\0";
        chooser.lpstrFile = names.data();
        chooser.nMaxFile = static_cast<DWORD>(names.size());
        chooser.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT |
                        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&chooser))
        {
            auto utf8 = [](const std::wstring& value) -> std::string {
                if (value.empty()) return {};
                const int size = WideCharToMultiByte(
                    CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                    nullptr, 0, nullptr, nullptr);
                std::string out(static_cast<std::size_t>(size), '\0');
                if (size > 0) WideCharToMultiByte(
                    CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                    &out[0], size, nullptr, nullptr);
                return out;
            };
            const wchar_t* cursor = names.data();
            const std::wstring directory(cursor);
            cursor += directory.size() + 1;
            if (*cursor == L'\0')
            {
                if (!m_import_paths.empty() && m_import_paths.back() != '\n')
                    m_import_paths += '\n';
                m_import_paths += utf8(directory) + "\n";
            }
            else
            {
                while (*cursor)
                {
                    const std::wstring file(cursor);
                    if (!m_import_paths.empty() && m_import_paths.back() != '\n')
                        m_import_paths += '\n';
                    m_import_paths += utf8(directory + L"\\" + file) + "\n";
                    cursor += file.size() + 1;
                }
            }
        }
    }
#endif
    ImGui::TextDisabled("Files are embedded in the JSON document.");
    ImGui::Separator();
    if (ImGui::Button("Import", ImVec2(100.0f, 0.0f)))
    {
        std::vector<definition_source> files;
        std::string errors;
        std::size_t begin = 0;
        while (begin < m_import_paths.size())
        {
            const std::size_t finish = m_import_paths.find('\n', begin);
            std::string path = m_import_paths.substr(begin, finish - begin);
            begin = (finish == std::string::npos)
                      ? m_import_paths.size() : finish + 1;
            const std::size_t start = path.find_first_not_of(" \t\r\n\"");
            const std::size_t last  = path.find_last_not_of(" \t\r\n\"");
            if (start == std::string::npos) continue;
            path = path.substr(start, last - start + 1);
            std::string body, error;
            if (read_file(path, body, error))
                files.push_back(definition_source{ path, body, false });
            else
                errors += path + ": " + error + "\n";
        }

        if (!files.empty())
        {
            const std::size_t last = files.size();
            edit("definitions.import", [&files](sheet& s) {
                for (definition_source& item : files)
                {
                    auto found = std::find_if(s.definitions.begin(),
                        s.definitions.end(),
                        [&item](const definition_source& in) {
                            return in.path == item.path;
                        });
                    if (found == s.definitions.end())
                        s.definitions.push_back(std::move(item));
                    else
                        found->text = std::move(item.text);
                }
            });
            doc().ui.definition =
                static_cast<int>(doc().doc.definitions.size()) - 1;
            doc().ui.definition_raise = true;
            status("imported " + format_dec(static_cast<u64>(last)) +
                   " definition file(s)" + (errors.empty() ? "" :
                   " (some failed: " + errors + ")"), !errors.empty());
            m_import_paths.clear();
            ImGui::CloseCurrentPopup();
        }
        else
            status(errors.empty() ? "Enter one or more file paths." : errors,
                   true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

/*
help_dialog
  Help's two pages, each a modal popup the window draws after the rest: the
keyboard shortcuts -- the keys shortcuts() answers, the "Add child..."
field's, and what the mouse does in the grid -- in a table; and what
bitwis(e)dom is. Close, or Escape, closes either.
*/
void
app::help_dialog()
{
    const char* const keys  = "Keyboard shortcuts##bitwisedom";
    const char* const about = "About bitwis(e)dom##bitwisedom";

    if (m_help_open)
    {
        ImGui::OpenPopup((m_help == 1) ? about : keys);
        m_help_open = false;
    }

    // close
    //   the Close button that ends a page; it, or Escape, closes the popup.
    auto close = []()
    {
        ImGui::Separator();

        if ( (ImGui::Button("Close", ImVec2(110.0f * ui_scale(), 0.0f))) ||
             (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) )
        {
            ImGui::CloseCurrentPopup();
        }

        return;
    };

    if (ImGui::BeginPopupModal(keys, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        // HELP_KEYS
        //   constant: each key or gesture, and what it does; a row with no
        // key is a heading.
        static const char* const HELP_KEYS[][2] = {
            { "", "The keyboard" },
            { "Ctrl+N", "a new sheet, in a window of its own" },
            { "Ctrl+O", "open a sheet, a bitfield or a C header" },
            { "Ctrl+W", "close the sheet in front" },
            { "Ctrl+S", "save the sheet, and its header beside it" },
            { "Ctrl+Shift+S", "save as" },
            { "Ctrl+E", "export the header" },
            { "Ctrl+Shift+E", "export everything as JSON" },
            { "Ctrl+Z", "undo" },
            { "Ctrl+Y, Ctrl+Shift+Z", "redo" },
            { "Ctrl+B", "add a bit" },
            { "Ctrl+G", "add a group" },
            { "Ctrl+Shift+A", "the Add node window: any kind of row, made "
                              "in full" },
            { "Ctrl+D", "duplicate the selected rows" },
            { "Del", "delete the selected rows" },
            { "Alt+Up, Alt+Down", "move the selected row up or down" },
            { "F2", "rename the selected row" },
            { "Ctrl+Shift+C", "copy the selected row as code" },
            { "Ctrl+Shift+V", "paste code after the selected row" },
            { "Ctrl+Shift+Plus", "expand every group (or Ctrl+keypad +)" },
            { "Ctrl+Shift+Minus",
              "collapse every group (or Ctrl+keypad -)" },
            { "Ctrl+J", "put the windows below the sheets away, or back" },
            { "Ctrl+=, Ctrl+-", "zoom the window in front in or out, as a "
                                "browser does (or Ctrl and the wheel)" },
            { "Ctrl+0", "the window in front at its actual size" },
            { "Ctrl+Alt+Right, Left", "the next or the last workspace" },
            { "Ctrl+,", "the Options" },
            { "G", "grid lines on or off" },
            { "Enter", "in an \"Add child...\" field: add a bit of that "
                       "name" },
            { "Escape", "in an \"Add child...\" field: clear it" },
            { "", "The mouse, in the grid" },
            { "click a name", "select its row; Ctrl adds, Shift extends" },
            { "click a cell", "set or move the row's bit there" },
            { "drag across cells", "define a range, or paint a literal" },
            { "Shift+drag", "select a rectangle of cells" },
            { "double-click a name", "rename it where it is" },
            { "click a band", "collapse or expand its segment" },
            { "right-click", "the menu for what is under the mouse" },
            { "drag a column's edge", "size it; double-click to fit it" },
            { "drag a ruler boundary",
              "size every cell; double-click to put them back" },
            { "symbol name, name, value",
              "the buttons over the symbols column: list each row by its "
              "symbol name, its own name or its value" },
            { "value, pressed again",
              "write the values another way, lined up: hex, decimal, "
              "binary, a shifted run, the bits set" },
            { "right-click a button", "every way to list the rows" },
            { "right-click the heading",
              "sort each group's rows: by name, value, lowest bit..." },
            { "right-click a row or a band",
              "show digits, names or colours there, or there only" },
            { "drag the probe or usage",
              "by its name, among the rows -- in a group, a usage row "
              "counts the group's -- or pin it, from its menu" },
            { "+ under a group's rows", "the Add node window, for it" },
            { "", "The windows" },
            { "drag a tab", "dock it beside, above or below another, or "
                            "float it, as Visual Studio does" },
            { "a tab's x", "close it; View > Tool windows opens it again" },
            { "double-click a sheet's tab", "rename its file" },
            { "the workspace list", "after the menus: switch workspaces, "
                                    "or add one" },
            { "the status bar's message",
              "the console: every action so far" } };

        if (ImGui::BeginTable("##keys",
                              2,
                              ImGuiTableFlags_SizingFixedFit |
                                  ImGuiTableFlags_RowBg))
        {
            for (const auto& each : HELP_KEYS)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                // a heading spans the row, in the accent
                if (each[0][0] == '\0')
                {
                    ImGui::TextColored(
                        ImGui::GetStyleColorVec4(ImGuiCol_CheckMark),
                        "%s",
                        each[1]);

                    continue;
                }

                ImGui::TextUnformatted(each[0]);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(each[1]);
            }

            ImGui::EndTable();
        }

        close();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(about, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushFont(nullptr, font_base() * 1.3f);
        ImGui::TextUnformatted("bitwis(e)dom");
        ImGui::PopFont();
        ImGui::TextUnformatted("A bitfield manager: symbols mapped to "
                               "values, shown across their bits, and");
        ImGui::TextUnformatted("written out as C and C++ macros -- and C "
                               "read back in as a sheet.");
        ImGui::TextDisabled("Built on djinterp, its window drawn by uxoxo "
                            "over Dear ImGui %s.",
                            IMGUI_VERSION);
        close();
        ImGui::EndPopup();
    }

    return;
}

void
app::export_header()
{
    const std::string stem = json_stem(doc().ui.path);
    const std::string& file = doc().doc.gen.file_name;
    const bool         hpp  = ( (file.size() >= 4) &&
                                (file.compare(file.size() - 4, 4, ".hpp") ==
                                 0) );
    const std::string  path = directory_of(doc().ui.path) +
                              (stem.empty() ? doc().doc.title : stem) +
                              (hpp ? ".hpp" : ".h");
    std::string error;
    if (!write_file(path, macro_header(), error))
    {
        status(error, true);
        return;
    }
    status("exported basic #defines to " + path, false);
}

/*
export_json
  The sheet as JSON beside its file: the bitfield alone, which a person or
a program can write as well as read, or everything -- the sheet, every
row as it resolves, the issues and the header.
*/
void
app::export_json(
    bool _everything
)
{
    const std::string stem = json_stem(doc().ui.path);
    const std::string path = directory_of(doc().ui.path) +
                             (stem.empty() ? doc().doc.title : stem) +
                             (_everything ? ".export.json"
                                          : ".bitfield.json");
    std::string       error;
    const std::string text =
        _everything ? write_export(doc().doc,
                                   doc().res,
                                   generate(doc().doc, doc().res,
                                            doc().today))
                    : write_bitfield(doc().doc);

    if (!write_file(path, text, error))
    {
        status(error, true);

        return;
    }

    status("wrote " + path, false);

    return;
}

/*
import_json_text
  JSON as a sheet: a sheet file opens as itself; a bitfield, or an export,
is a new sheet beside the file it came from -- or, from the clipboard,
beside the sheet that was open -- named for it, and not yet saved. What
the reading passed over is listed on the Issues page.
*/
void
app::import_json_text(
    const std::string& _path,
    const std::string& _text
)
{
    json_read   read = read_json_sheet(_text);
    std::string name;
    std::string stem;
    std::string extension;

    file_parts(_path, name, stem, extension);

    const std::string from = _path.empty() ? std::string("the clipboard")
                                           : name;

    if (!read.ok)
    {
        status(from + ": " + read.error, true);

        return;
    }

    if ( (read.shape == json_shape::sheet) &&
         (!_path.empty()) )
    {
        (void)fresh_document();
        load_sheet_into(std::move(read.result), _path);
        status("opened " + _path, false);

        return;
    }

    const std::string base  = _path.empty() ? read.result.title
                                            : json_stem(_path);
    const std::string where = directory_of(_path.empty() ? doc().ui.path
                                                         : _path);
    const char*       shape = (read.shape == json_shape::bitfield)
                                  ? "a bitfield"
                              : (read.shape == json_shape::everything)
                                  ? "an export"
                                  : "a sheet";
    int               rows  = 0;

    walk(read.result,
         [&rows](const row&, const row&, int)
         {
             ++rows;
         });

    (void)fresh_document();
    load_sheet_into(std::move(read.result), where + base + ".bitwisedom.json");

    doc().ui.dirty        = true;
    doc().ui.import_name  = from;
    doc().ui.import_notes = read.notes;
    doc().ui.fit_columns  = true;

    status("read " + from + ", " + shape + ": " +
               format_dec(static_cast<u64>(rows)) + " row(s)" +
               (read.notes.empty()
                    ? std::string()
                    : "; " + format_dec(read.notes.size()) +
                          " key(s) passed over, on the Issues page"),
           false);

    return;
}

// ------------------------------------------- the menu, renames and code

/*
open_menu
  The row right-clicked is the row the menu acts on: it is selected, and
put first in the selection, which is the one the inspector shows and an
addition goes beside.
*/
void
app::open_menu(
    const std::string&       _kind,
    const std::string&       _where,
    const uxoxo::option_set& _payload
)
{
    ui_state& ui = doc().ui;

    ui.menu_kind  = _kind;
    ui.menu_where = _where;
    ui.menu_open  = true;

    if (_kind == "row")
    {
        const int id = static_cast<int>(_payload.as_long("value", 0));

        if (find_row(doc().doc, id) == nullptr)
        {
            ui.menu_open = false;

            return;
        }

        std::vector<int>& selection = ui.selection;
        const auto        found     = std::find(selection.begin(),
                                                selection.end(),
                                                id);

        if (found == selection.end())
        {
            selection.assign(1, id);
        }
        else
        {
            std::rotate(selection.begin(), found, found + 1);
        }

        ui.menu_row           = id;
        ui.anchor             = id;
        doc().preview_valid = false;
    }
    else if (_kind == "segment")
    {
        ui.menu_segment = static_cast<int>(_payload.as_long("value", -1));
        ui.menu_lsb     = static_cast<int>(_payload.as_long("lsb", 0));
        ui.menu_msb     = static_cast<int>(_payload.as_long("msb", 0));
    }
    else if (_kind == "special")
    {
        ui.menu_special = static_cast<int>(_payload.as_long("value", 0));

        if (find_special(doc().doc.grid, ui.menu_special) == nullptr)
        {
            ui.menu_open = false;
        }
    }

    ++doc().version;

    return;
}

/*
begin_rename
  A row is renamed where it was asked for: in the tree, whose groups around
it are opened so that it shows, or in the grid. It is selected, and the
field takes the keyboard.
*/
void
app::begin_rename(
    int                _id,
    const std::string& _where
)
{
    const row* found = find_row(doc().doc, _id);

    if ( (found == nullptr) ||
         (_id == 0) )
    {
        return;
    }

    // the tree is the grid's symbols column while its own window is
    // closed: the row is renamed there
    const std::string where =
        ( (_where == "tree") &&
          (!current().tools[static_cast<std::size_t>(tool::symbols)]) )
            ? std::string("grid")
            : _where;

    // the groups around it, opened
    bool closed = false;

    for (row* parent = parent_of(doc().doc, _id);
         ( (parent != nullptr) && (parent->id != 0) );
         parent = parent_of(doc().doc, parent->id))
    {
        closed = ( (closed) || (parent->collapsed) );
    }

    if (closed)
    {
        const int id = _id;

        quietly([id](sheet& _sheet)
                {
                    for (row* parent = parent_of(_sheet, id);
                         ( (parent != nullptr) && (parent->id != 0) );
                         parent = parent_of(_sheet, parent->id))
                    {
                        parent->collapsed = false;
                    }
                });
    }

    ui_state& ui = doc().ui;

    ui.selection.assign(1, _id);
    ui.anchor           = _id;
    ui.renaming         = _id;
    ui.renaming_segment = -1;
    ui.rename_text      = find_row(doc().doc, _id)->name;
    ui.rename_where     = where;
    ui.rename_focus     = true;
    ++doc().version;

    return;
}

/*
finish_rename
  The rename is tried on a copy first: a name that is no C identifier, or
another symbol's, leaves the sheet and its history as they were, and the
status line says why.
*/
void
app::finish_rename(
    int                _id,
    const std::string& _text
)
{
    doc().ui.renaming = 0;
    ++doc().version;

    const row* found = find_row(doc().doc, _id);

    if ( (found == nullptr) ||
         (found->name == _text) )
    {
        return;
    }

    const std::string before = found->name;
    sheet             trial  = doc().doc;
    const renamed     done   = rename_row(trial, _id, _text);

    if (!done.ok)
    {
        status(done.error, true);

        return;
    }

    edit(std::string(),
         [&trial](sheet& _sheet)
         {
             _sheet = std::move(trial);
         });

    status("renamed " + (before.empty() ? std::string("(unnamed)") : before) +
               " to " + _text +
               ((done.references > 0)
                    ? "; " + format_dec(static_cast<u64>(done.references)) +
                          " reference(s) rewritten"
                    : std::string()),
           false);

    return;
}

/*
begin_segment_rename
  A band the sheet made itself -- a byte, say -- has no name to keep, so
renaming it first makes it a segment of the sheet's own.
*/
void
app::begin_segment_rename(
    const uxoxo::option_set& _payload
)
{
    long      index = _payload.as_long("value", -1);
    const int lsb   = static_cast<int>(_payload.as_long("lsb", 0));
    const int msb   = static_cast<int>(_payload.as_long("msb", 0));

    if (index < 0)
    {
        edit(std::string(),
             [lsb, msb](sheet& _sheet)
             {
                 column_segment made;
                 made.name = format_dec(static_cast<u64>(msb)) + ":" +
                             format_dec(static_cast<u64>(lsb));
                 made.lsb  = lsb;
                 made.msb  = msb;
                 _sheet.segments.push_back(made);
             });

        index = static_cast<long>(doc().doc.segments.size()) - 1;
    }

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= doc().doc.segments.size()) )
    {
        return;
    }

    ui_state& ui = doc().ui;

    ui.renaming         = 0;
    ui.renaming_segment = static_cast<int>(index);
    ui.rename_text      =
        doc().doc.segments[static_cast<std::size_t>(index)].name;
    ui.rename_where     = "grid";
    ui.rename_focus     = true;
    ++doc().version;

    return;
}

void
app::finish_segment_rename(
    const std::string& _text
)
{
    const int index = doc().ui.renaming_segment;

    doc().ui.renaming_segment = -1;
    ++doc().version;

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= doc().doc.segments.size()) ||
         (doc().doc.segments[static_cast<std::size_t>(index)].name ==
          _text) )
    {
        return;
    }

    edit(std::string(),
         [index, &_text](sheet& _sheet)
         {
             _sheet.segments[static_cast<std::size_t>(index)].name = _text;
         });

    return;
}

/*
copy_code
  The selection as code to paste elsewhere when the row asked about is in
it, the row alone when it is not, and the whole sheet for none.
*/
void
app::copy_code(
    int _id
)
{
    std::vector<int> rows;

    if (_id == 0)
    {
        for (const row& child : doc().doc.root.children)
        {
            rows.push_back(child.id);
        }
    }
    else if (is_selected(doc(), _id))
    {
        rows = doc().ui.selection;
    }
    else
    {
        rows.push_back(_id);
    }

    const generated code = generate_code(doc().doc, doc().res, rows);

    ImGui::SetClipboardText(code.text.c_str());
    status(format_dec(static_cast<u64>(code.macros)) +
               " definition(s) on the clipboard, as code",
           false);

    return;
}

/*
paste_code
  The clipboard's #defines read at the sheet's level and merged into it:
beside the row asked about, or into it when it is a group. A name the
sheet has already takes the pasted definition.
*/
void
app::paste_code(
    int _id
)
{
    const char*    clipboard = ImGui::GetClipboardText();
    import_options options;

    options.level = doc().doc.gen.level;
    options.model = doc().doc.gen.model;
    options.title = doc().doc.title;

    const imported in = import_code((clipboard != nullptr) ? clipboard : "",
                                    options);

    if (!in.ok)
    {
        status("the clipboard holds no #define to paste", true);

        return;
    }

    merged result;

    edit(std::string(),
         [&result, &in, _id](sheet& _sheet)
         {
             result = merge_rows(_sheet, _id, in.result);
         });

    doc().ui.selection    = result.ids;
    doc().ui.import_name  = "the clipboard";
    doc().ui.import_notes = in.notes;
    doc().ui.import_notes.insert(doc().ui.import_notes.end(),
                                   result.notes.begin(),
                                   result.notes.end());
    ++doc().version;

    status("pasted " + format_dec(static_cast<u64>(in.definitions)) +
               " definition(s): " +
               format_dec(static_cast<u64>(result.added)) + " added, " +
               format_dec(static_cast<u64>(result.updated)) + " replaced",
           false);

    return;
}

/*
import_code_text
  Code made a new sheet, saved -- when it is saved -- beside the code it
came from, under its name. A C++ file is read as C++, at C++17 when the
sheet was C; the rest at the sheet's own level.
*/
void
app::import_code_text(
    const std::string& _path,
    const std::string& _text
)
{
    std::string name;
    std::string stem;
    std::string extension;

    file_parts(_path, name, stem, extension);

    const bool cpp_file = ( (extension == "hpp") || (extension == "hh")  ||
                            (extension == "hxx") || (extension == "h++") ||
                            (extension == "cpp") || (extension == "cc")  ||
                            (extension == "cxx") || (extension == "ipp") ||
                            (extension == "inl") );
    const bool header   = ( (cpp_file) ||
                            (extension == "h") );

    import_options options;
    options.level     = ( (cpp_file) && (!is_cpp(doc().doc.gen.level)) )
                            ? standard::cpp17
                            : doc().doc.gen.level;
    options.model     = doc().doc.gen.model;
    options.title     = stem.empty() ? std::string("pasted") : stem;
    options.file_name = header ? name : options.title + ".h";

    imported in = import_code(_text, options);

    if (!in.ok)
    {
        status((_path.empty() ? std::string("the clipboard") : _path) +
                   " has no #define to import",
               true);

        return;
    }

    const std::string sheet_path = directory_of(_path) + options.title +
                                   ".bitwisedom.json";

    (void)fresh_document();
    load_sheet_into(std::move(in.result), sheet_path);

    doc().ui.dirty           = true;
    doc().ui.import_name     = _path.empty() ? std::string("the clipboard")
                                               : name;
    doc().ui.import_notes    = in.notes;
    doc().ui.fit_columns     = true;

    status("imported " + doc().ui.import_name + ": " +
               format_dec(static_cast<u64>(in.definitions)) +
               " definition(s) -- " +
               format_dec(static_cast<u64>(in.literals)) + " literal(s), " +
               format_dec(static_cast<u64>(in.allocations)) +
               " bit(s) or range(s), " +
               format_dec(static_cast<u64>(in.macros)) + " macro(s) -- in " +
               format_dec(static_cast<u64>(in.groups)) +
               " group(s); see Issues",
           false);

    return;
}

// grid_width
//   a column of the grid set to a width: the name or value column fitted
// to its text, or the bits' cells.
void
app::grid_width(
    const uxoxo::option_set& _payload
)
{
    const std::string column = _payload.as_string("target", "name");
    const float       width  =
        static_cast<float>(as_number(_payload, "value", 0.0));

    // a fit as a sheet arrives is part of its arriving: the sheet is no
    // more changed than it was, nor any less new
    const bool arriving = _payload.as_bool("arriving", false);
    const bool dirty    = doc().ui.dirty;
    const bool pristine = doc().ui.pristine;
    const bool browsing = doc().ui.browsing;

    if (width <= 0.0f)
    {
        return;
    }

    const bool reset = _payload.as_bool("reset", false);

    quietly([&column, width, reset](sheet& _sheet)
            {
                if (column == "value")
                {
                    _sheet.grid.value_width =
                        std::max(within_setting("grid.value_width", width),
                                 VALUE_FLOOR);
                }
                else if (column == "cells")
                {
                    // sized by hand, the cells keep the width: the room to
                    // spare no longer widens them
                    _sheet.grid.cell_width =
                        within_setting("grid.cell_width", width);
                    _sheet.grid.fill = reset ? grid_fill::cells
                                             : grid_fill::none;
                }
                else
                {
                    _sheet.grid.name_width =
                        within_setting("grid.name_width", width);
                }
            });

    if (arriving)
    {
        doc().ui.dirty    = dirty;
        doc().ui.pristine = pristine;
        doc().ui.browsing = browsing;
    }

    return;
}

// -------------------------------------------------------------- operations

void
app::apply_operation()
{
    const op_kind op   = static_cast<op_kind>(
        std::clamp(doc().ui.op, 0, OP_COUNT - 1));
    const op_info info = describe(op);
    wide          operand;

    if ( (!sheet_number(doc().ui.operand, doc().doc.width, operand)) &&
         (info.operand[0] != '\0') )
    {
        status("the operand is not a number: " +
                   parse_wide(doc().ui.operand).error,
               true);

        return;
    }

    const std::vector<int> targets = op_targets(doc());
    const bool             probe   = doc().ui.op_probe;
    const resolution       before  = doc().res;
    int                    count   = 0;

    edit(std::string(),
         [&](sheet& _sheet)
         {
             count = apply_op_to(_sheet,
                                 before,
                                 targets,
                                 op,
                                 operand,
                                 probe);
         });

    status(std::string(info.name) + ": " +
               format_dec(static_cast<u64>(count)) + " changed",
           false);

    return;
}

}  // namespace gui
}  // namespace bitwisedom
