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
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./app.hpp"  // corresponding header

// std
#include <algorithm>     // std::find, std::remove, std::clamp, std::rotate
#include <cctype>        // std::tolower
#include <cstddef>       // std::size_t
#include <cstdio>        // std::snprintf
#include <cmath>         // std::isfinite
#include <cstdlib>       // std::getenv, std::strtod, std::strtof
#include <cstring>       // std::strcmp, std::strlen, std::strncmp
#include <ctime>         // std::time, std::localtime
#include <filesystem>    // std::filesystem::create_directories
#include <system_error>  // std::error_code
#include <utility>       // std::move
#include <variant>       // std::get_if
// imgui
#include <imgui.h>           // ImGui
#include <imgui_internal.h>  // ImGuiSettingsHandler, AddSettingsHandler
// uxoxo
#include <uxoxo/realize.hpp>  // realize
// bitwisedom
#include <djinterp/tools/bitwisedom/cimport.hpp>   // import_code, merge_rows
#include <djinterp/tools/bitwisedom/examples.hpp>  // make_example
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet, write_export
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
//   the rows the grid and the tree show, in order.
std::vector<int>
visible_order(
    const model& _model
)
{
    std::vector<int> out;

    for (const resolved_row& each : _model.res.rows)
    {
        if (each.visible)
        {
            out.push_back(each.id);
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
//   struct: what the settings file holds of the window -- the panels' sizes,
// unscaled, the theme and scheme, and how the panels are shown -- apart from
// the window, which ImGui's context may outlive.
struct remembered
{
    float left       = 0.0f;
    float right      = 0.0f;
    float bottom     = 0.0f;
    int   theme      = 0;
    int   scheme     = 0;
    int   panel      = 0;
    int   panel_back = 0;
    int   symbols    = 1;
    int   inspector  = 1;
    bool  read       = false;   // the sizes
    bool  viewed     = false;   // the rest
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

    return (std::strcmp(_name, "layout") == 0) ? &memory() : nullptr;
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

    // read
    //   one "key=value" line into its field
    auto read = [_line](const char* _key,
                        float&      _field)
    {
        const std::size_t length = std::strlen(_key);

        if ( (std::strncmp(_line, _key, length) == 0) &&
             (_line[length] == '=') )
        {
            const float value = std::strtof(_line + length + 1, nullptr);

            if (value > 0.0f)
            {
                _field        = value;
                memory().read = true;
            }
        }

        return;
    };

    // read_int
    //   one "key=value" line into a small whole number, held to its range
    auto read_int = [_line](const char* _key,
                            int&        _field,
                            int         _count)
    {
        const std::size_t length = std::strlen(_key);

        if ( (std::strncmp(_line, _key, length) == 0) &&
             (_line[length] == '=') )
        {
            const long value = std::strtol(_line + length + 1, nullptr, 10);

            if ( (value >= 0) &&
                 (value < _count) )
            {
                _field          = static_cast<int>(value);
                memory().viewed = true;
            }
        }

        return;
    };

    read("left", memory().left);
    read("right", memory().right);
    read("bottom", memory().bottom);
    read_int("theme", memory().theme, THEME_COUNT);
    read_int("scheme", memory().scheme, SCHEME_COUNT);
    read_int("panel", memory().panel, PANEL_MODE_COUNT);
    read_int("panel_back", memory().panel_back, PANEL_MODE_COUNT);
    read_int("symbols", memory().symbols, 2);
    read_int("inspector", memory().inspector, 2);

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

    if (held.left <= 0.0f)
    {
        return;
    }

    _out->appendf("[%s][layout]\n", _handler->TypeName);
    _out->appendf("left=%.1f\nright=%.1f\nbottom=%.1f\n",
                  static_cast<double>(held.left),
                  static_cast<double>(held.right),
                  static_cast<double>(held.bottom));
    _out->appendf("theme=%d\nscheme=%d\npanel=%d\npanel_back=%d\n"
                  "symbols=%d\ninspector=%d\n\n",
                  held.theme,
                  held.scheme,
                  held.panel,
                  held.panel_back,
                  held.symbols,
                  held.inspector);

    return;
}

// remember
//   the panels' sizes, the theme and scheme, and how the panels are shown,
// to be written to the settings file.
void
remember(
    const ui_state& _ui
)
{
    const float scale = ui_scale();

    memory().left       = _ui.left / scale;
    memory().right      = _ui.right / scale;
    memory().bottom     = _ui.bottom / scale;
    memory().theme      = _ui.theme;
    memory().scheme     = _ui.scheme;
    memory().panel      = static_cast<int>(_ui.panel);
    memory().panel_back = static_cast<int>(_ui.panel_back);
    memory().symbols    = _ui.show_left ? 1 : 0;
    memory().inspector  = _ui.show_right ? 1 : 0;

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


std::string
preferences_path()
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

    // as UTF-8, which is how ImGui opens a file by name
    const auto named = (where / "bitwisedom.ini").u8string();

    return std::string(named.begin(), named.end());
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
op_targets(
    const model& _model
)
{
    return _model.ui.selection;
}


// ------------------------------------------------------------------- the app

app::app(
    std::string _preferences
)
    : m_preferences(std::move(_preferences))
{
    const float scale = ui_scale();

    m_model.ui.left   *= scale;
    m_model.ui.right  *= scale;
    m_model.ui.bottom *= scale;
    m_model.today      = today();
    load_sheet_into(make_example("uart_cr"), "uart_cr.bitwisedom.json");
    status("uart_cr: an example -- File > Examples has two more", false);
    register_renderers(m_model);

    // the panels' sizes, kept in ImGui's settings file beside the tables'
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

    // the settings file, read before the first frame: ImGui's tables, and
    // the panels' sizes through the handler above
    std::string text;
    std::string error;

    if ( (!m_preferences.empty())                  &&
         (ImGui::GetCurrentContext() != nullptr)   &&
         (read_file(m_preferences, text, error)) )
    {
        const std::string kept = readable(text);

        ImGui::LoadIniSettingsFromMemory(kept.c_str(), kept.size());
    }
}

app::~app()
{
    keep_preferences();

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
    return m_model.ui.quit;
}

const model&
app::state() const
{
    return m_model;
}

void
app::post(
    const event& _event
)
{
    m_queue.push_back(_event);

    return;
}

void
app::status(
    const std::string& _text,
    bool               _error
)
{
    m_model.ui.status       = _text;
    m_model.ui.status_error = _error;
    ++m_model.version;

    return;
}

void
app::changed()
{
    m_model.shown         = make_look(m_model.doc,
                                      m_model.ui.theme,
                                      m_model.ui.scheme);
    m_model.res           = resolve(m_model.doc, m_model.shown.colours);
    m_model.preview_valid = false;
    m_model.header_valid  = false;
    ++m_model.version;

    // a selected row that no longer exists is no longer selected
    std::vector<int>& selection = m_model.ui.selection;

    selection.erase(std::remove_if(selection.begin(),
                                   selection.end(),
                                   [this](int _id)
                                   {
                                       return (m_model.res.find(_id) ==
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
    m_model.steps.record(m_model.doc, _key);
    _change(m_model.doc);
    m_model.ui.dirty = true;
    changed();

    return;
}

template<typename Change>
void
app::quietly(
    Change _change
)
{
    _change(m_model.doc);
    m_model.ui.dirty = true;
    changed();

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
    const int count = open_groups(m_model.doc, _group, _open);

    if (count > 0)
    {
        m_model.ui.dirty = true;
        changed();
    }

    // what was asked for: every group, or one and the groups within it
    const row*        group = find_row(m_model.doc, _group);
    const bool        every = (_group == m_model.doc.root.id);
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

/*
panel_to
  The bottom panel shown another way. A panel hidden remembers how it was
shown, for Ctrl+J to bring it back so; a panel that peeks starts closed.
*/
void
app::panel_to(
    panel_mode _mode
)
{
    ui_state& ui = m_model.ui;

    if ( (_mode == panel_mode::hidden) &&
         (ui.panel != panel_mode::hidden) )
    {
        ui.panel_back = ui.panel;
        status("the panel is hidden: View > Bottom panel, or Ctrl+J, "
               "brings it back",
               false);
    }

    // the panel's tab bar is a new one, which would show its first page:
    // the page in front is brought to the front of it
    ui.panel      = _mode;
    ui.peeking    = false;
    ui.tab_forced = true;
    remember(ui);
    ++m_model.version;

    return;
}

void
app::load_sheet_into(
    sheet              _sheet,
    const std::string& _path
)
{
    m_model.doc = std::move(_sheet);
    m_model.steps.clear();
    m_model.ui.selection.clear();
    m_model.ui.grid_rect_rows.clear();
    m_model.ui.grid_rect_low = -1;
    m_model.ui.grid_rect_high = -1;
    m_model.ui.path  = _path;
    m_model.ui.dirty = false;
    changed();

    return;
}

/*
frame
  The interface is realized again only when the model has moved on; the
style is set again only when the theme or accent has.
*/
void
app::frame()
{
    // the panels' sizes the settings file remembered, once it is read
    if ( (!m_remembered) &&
         (memory().read) )
    {
        const float scale = ui_scale();

        m_model.ui.left   = std::clamp(memory().left * scale, 140.0f, 900.0f);
        m_model.ui.right  = std::clamp(memory().right * scale, 200.0f, 900.0f);
        m_model.ui.bottom = std::clamp(memory().bottom * scale, 90.0f,
                                       1200.0f);
        ++m_model.version;
    }

    // the theme, the scheme and the panels, likewise
    if ( (!m_remembered) &&
         (memory().viewed) )
    {
        ui_state& ui = m_model.ui;

        ui.theme      = memory().theme;
        ui.scheme     = memory().scheme;
        ui.panel      = static_cast<panel_mode>(memory().panel);
        ui.panel_back = static_cast<panel_mode>(memory().panel_back);
        ui.show_left  = (memory().symbols != 0);
        ui.show_right = (memory().inspector != 0);
        changed();
    }

    m_remembered = true;

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
        m_model.ui.theme * 1000003 +
        m_model.ui.scheme * 7919 +
        static_cast<int>(m_model.shown.colours.accent & 0xFFFFFu);

    if (style != m_styled)
    {
        apply_style(m_model.shown, m_model.ui.scheme);
        m_styled = style;
    }

    // the caches the interface draws from, if a change dropped them
    if (!m_model.preview_valid)
    {
        wide operand;

        (void)sheet_number(m_model.ui.operand, m_model.doc.width, operand);

        m_model.preview = preview_op(
            m_model.doc,
            m_model.res,
            op_targets(m_model),
            static_cast<op_kind>(std::clamp(m_model.ui.op, 0, OP_COUNT - 1)),
            operand,
            m_model.ui.op_probe);
        m_model.preview_valid = true;
    }

    if (!m_model.header_valid)
    {
        m_model.header       = generate(m_model.doc,
                                        m_model.res,
                                        m_model.today);
        m_model.header_valid = true;
    }

    if (m_built != m_model.version)
    {
        m_blueprint = uxoxo::realize<uxoxo::platform::imgui_platform>(
            build_ui(m_model));
        m_built     = m_model.version;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("bitwisedom",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration          |
                 ImGuiWindowFlags_NoMove                |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    m_model.ui.hover_row = 0;
    m_model.ui.hover_bit = -1;

    uxoxo::platform::imgui_frame posted;

    if (m_blueprint.draw)
    {
        m_blueprint.draw(posted);
    }

    shortcuts(posted);
    ImGui::End();

    if (m_model.ui.demo)
    {
        ImGui::ShowDemoWindow(&m_model.ui.demo);
    }

    // a page forced to the front, a menu opened, a field focused and the
    // columns fitted are so for one frame only
    if ( ( (m_model.ui.tab_forced)   ||
           (m_model.ui.menu_open)    ||
           (m_model.ui.rename_focus) ||
           (m_model.ui.fit_columns) ) &&
         (m_built == m_model.version) )
    {
        m_model.ui.tab_forced   = false;
        m_model.ui.menu_open    = false;
        m_model.ui.rename_focus = false;
        m_model.ui.fit_columns  = false;
        ++m_model.version;
    }

    for (const event& each : posted.events)
    {
        apply(each);
    }

    if (ImGui::GetIO().WantSaveIniSettings)
    {
        keep_preferences();
    }

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
    chord(ImGuiMod_Ctrl | ImGuiKey_O, "file.open");
    chord(ImGuiMod_Ctrl | ImGuiKey_E, "file.export");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E,
          "file.export_everything");
    chord(ImGuiMod_Ctrl | ImGuiKey_D, "edit.duplicate");
    chord(ImGuiKey_Delete, "edit.delete");
    chord(ImGuiMod_Ctrl | ImGuiKey_G, "edit.add_group");
    chord(ImGuiMod_Ctrl | ImGuiKey_B, "edit.add_bit");
    chord(ImGuiMod_Alt | ImGuiKey_UpArrow, "edit.move_up");
    chord(ImGuiMod_Alt | ImGuiKey_DownArrow, "edit.move_down");
    chord(ImGuiMod_Ctrl | ImGuiKey_J, "panel.toggle");

    // every group opened or closed: Ctrl+Shift with the main keyboard's
    // plus or minus, or Ctrl with the keypad's
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Equal, "tree.open_all");
    chord(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd, "tree.open_all");
    chord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Minus, "tree.close_all");
    chord(ImGuiMod_Ctrl | ImGuiKey_KeypadSubtract, "tree.close_all");

    // the selected row: renamed, copied as code, or code pasted beside it
    const long selected = m_model.ui.selection.empty()
                              ? 0L
                              : static_cast<long>(
                                    m_model.ui.selection.front());

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
        payload.set("value", !m_model.doc.grid.lines);
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

    // settings, by path
    if (kind == "set")
    {
        const std::string path  = payload.as_string("target", "");
        const setting*    found = find_setting(path);

        if (found != nullptr)
        {
            edit("set:" + path,
                 [found, &payload](sheet& _sheet)
                 {
                     found->write(_sheet, payload);
                 });
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
        if (m_model.steps.undo(m_model.doc))
        {
            changed();
            status("undone", false);
        }
    }
    else if (kind == "edit.redo")
    {
        if (m_model.steps.redo(m_model.doc))
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
        const std::vector<int> order = visible_order(m_model);
        const auto first = std::find(order.begin(), order.end(), from);
        const auto last  = std::find(order.begin(), order.end(), to);
        if ((first != order.end()) && (last != order.end()))
        {
            m_model.ui.grid_rect_rows.assign(
                 std::min(first, last), std::max(first, last) + 1);
            m_model.ui.selection = m_model.ui.grid_rect_rows;
            m_model.ui.grid_rect_low = static_cast<int>(
                 payload.as_long("low", 0));
            m_model.ui.grid_rect_high = static_cast<int>(
                 payload.as_long("high", 0));
            m_model.ui.anchor = from;
            m_model.preview_valid = false;
            ++m_model.version;
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
    // --------------------------------------------------------------- layout
    else if ( (kind == "layout.left") ||
              (kind == "layout.right") ||
              (kind == "layout.bottom") )
    {
        const float delta =
            static_cast<float>(as_number(payload, "value", 0.0));

        if (kind == "layout.left")
        {
            m_model.ui.left = std::clamp(m_model.ui.left + delta,
                                         140.0f,
                                         900.0f);
        }
        else if (kind == "layout.right")
        {
            m_model.ui.right = std::clamp(m_model.ui.right - delta,
                                          200.0f,
                                          900.0f);
        }
        else
        {
            m_model.ui.bottom = std::clamp(m_model.ui.bottom - delta,
                                           90.0f,
                                           1200.0f);
        }

        remember(m_model.ui);
        ++m_model.version;
    }
    else if (kind == "tab")
    {
        // a page the user brought to the front
        m_model.ui.tab        = static_cast<int>(value);
        m_model.ui.tab_forced = false;
        ++m_model.version;
    }
    else if (kind == "tab.show")
    {
        // a page asked for from elsewhere: ImGui's tab bar keeps its own
        // choice, so the page is brought to the front for one frame -- and
        // a panel out of sight is brought back to show it
        m_model.ui.tab        = static_cast<int>(value);
        m_model.ui.tab_forced = true;

        if ( (m_model.ui.panel == panel_mode::minimized) ||
             (m_model.ui.panel == panel_mode::hidden) )
        {
            panel_to(panel_mode::docked);
        }
        else if (m_model.ui.panel == panel_mode::peek)
        {
            m_model.ui.peeking = true;
        }

        ++m_model.version;
    }
    // ------------------------------------------------- the view's choices
    else if (kind == "view.theme")
    {
        m_model.ui.theme = std::clamp(static_cast<int>(value), 0,
                                      THEME_COUNT - 1);
        remember(m_model.ui);
        changed();
    }
    else if (kind == "view.scheme")
    {
        m_model.ui.scheme = std::clamp(static_cast<int>(value), 0,
                                       SCHEME_COUNT - 1);
        remember(m_model.ui);
        changed();
    }
    else if (kind == "view.symbols")
    {
        m_model.ui.show_left = payload.as_bool("value",
                                               !m_model.ui.show_left);
        remember(m_model.ui);
        ++m_model.version;
    }
    else if (kind == "view.inspector")
    {
        m_model.ui.show_right = payload.as_bool("value",
                                                !m_model.ui.show_right);
        remember(m_model.ui);
        ++m_model.version;
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
    // ----------------------------------------------------- the bottom panel
    else if (kind == "panel.mode")
    {
        panel_to(static_cast<panel_mode>(std::clamp(
            static_cast<int>(value), 0, PANEL_MODE_COUNT - 1)));
    }
    else if (kind == "panel.toggle")
    {
        // hidden and back, as it was
        panel_to( (m_model.ui.panel == panel_mode::hidden)
                      ? ( (m_model.ui.panel_back == panel_mode::hidden)
                              ? panel_mode::docked
                              : m_model.ui.panel_back )
                      : panel_mode::hidden );
    }
    else if (kind == "panel.open")
    {
        // a tab pressed on a strip: its page, shown
        m_model.ui.tab        = static_cast<int>(value);
        m_model.ui.tab_forced = true;

        if (m_model.ui.panel == panel_mode::peek)
        {
            m_model.ui.peeking = true;
        }
        else
        {
            panel_to(panel_mode::docked);
        }

        ++m_model.version;
    }
    else if (kind == "panel.peek")
    {
        if ( (m_model.ui.panel == panel_mode::peek) &&
             (!m_model.ui.peeking) )
        {
            m_model.ui.peeking = true;
            ++m_model.version;
        }
    }
    else if (kind == "panel.leave")
    {
        if (m_model.ui.peeking)
        {
            m_model.ui.peeking = false;
            ++m_model.version;
        }
    }
    else if (kind == "panel.close")
    {
        panel_to(panel_mode::hidden);
    }
    // ---------------------------------------------------------- operations
    else if (kind == "ops.op")
    {
        m_model.ui.op         = static_cast<int>(value);
        m_model.preview_valid = false;
        ++m_model.version;
    }
    else if (kind == "ops.operand")
    {
        m_model.ui.operand    = payload.as_string("value", "");
        m_model.preview_valid = false;
        ++m_model.version;
    }
    else if (kind == "ops.probe")
    {
        m_model.ui.op_probe   = payload.as_bool("value", false);
        m_model.preview_valid = false;
        ++m_model.version;
    }
    else if (kind == "ops.preview")
    {
        m_model.ui.op_preview = payload.as_bool("value", true);
        ++m_model.version;
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
                         m_model.doc.width,
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
        const resolved_row* found = m_model.res.find(static_cast<int>(value));

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
        m_model.ui.path = payload.as_string("value", m_model.ui.path);
        ++m_model.version;
    }
    else if (kind == "file.new")
    {
        load_sheet_into(make_document("untitled", 32),
                        "untitled.bitwisedom.json");
        m_model.ui.selection.assign(1, m_model.doc.root.children.front().id);
        m_model.ui.anchor = m_model.ui.selection.front();
        ++m_model.version;
        status("a new sheet", false);
    }
    else if (kind == "file.example")
    {
        const std::vector<std::string> names = example_names();
        const std::size_t              index =
            static_cast<std::size_t>(std::clamp<long>(
                value, 0, static_cast<long>(names.size()) - 1));

        load_sheet_into(make_example(names[index]),
                        names[index] + ".bitwisedom.json");
        m_model.ui.fit_columns = true;
        status("the " + names[index] + " example", false);
    }
    else if (kind == "file.open")
    {
        open_file();
    }
    else if (kind == "file.save")
    {
        save_file();
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
        ImGui::SetClipboardText(write_bitfield(m_model.doc).c_str());
        status("the bitfield is on the clipboard, as JSON", false);
    }
    else if (kind == "gen.copy")
    {
        const generated header = generate(m_model.doc,
                                          m_model.res,
                                          m_model.today);

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
        m_model.ui.renaming         = 0;
        m_model.ui.renaming_segment = -1;
        ++m_model.version;
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
        m_model.ui.path = payload.as_string("value", m_model.ui.path);
        open_file();
    }
    // --------------------------------------------------------- the grid's
    else if (kind == "grid.width")
    {
        grid_width(payload);
    }
    else if (kind == "app.quit")
    {
        m_model.ui.quit = true;
    }
    else if (kind == "app.demo")
    {
        m_model.ui.demo = payload.as_bool("value", !m_model.ui.demo);
        ++m_model.version;
    }

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
    std::vector<int>& selection = m_model.ui.selection;
    m_model.ui.grid_rect_rows.clear();
    m_model.ui.grid_rect_low = -1;
    m_model.ui.grid_rect_high = -1;

    if ( (io.KeyShift) &&
         (m_model.ui.anchor != 0) )
    {
        const std::vector<int> order = visible_order(m_model);
        const auto             from  = std::find(order.begin(),
                                                 order.end(),
                                                 m_model.ui.anchor);
        const auto             to    = std::find(order.begin(),
                                                 order.end(),
                                                 _id);

        if ( (from != order.end()) &&
             (to != order.end()) )
        {
            selection.assign((from < to) ? from : to,
                             ((from < to) ? to : from) + 1);
            ++m_model.version;

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

    m_model.ui.anchor     = _id;
    m_model.preview_valid = false;
    ++m_model.version;

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
    const row* selected = inspected(m_model);
    const int  parent   = (selected != nullptr) ? selected->id : 0;
    row        made;

    if (_kind < 0)
    {
        made.kind = row_kind::group;
        made.name = unique_name(m_model.doc, "Group");
    }
    else
    {
        made.def = static_cast<def_kind>(std::clamp(_kind,
                                                    0,
                                                    DEF_KIND_COUNT - 1));

        const int free = next_free_bit(m_model.doc, parent);

        switch (made.def)
        {
            case def_kind::bit:
                made.name = unique_name(m_model.doc, "FLAG");
                made.bit  = (free < 0) ? 0 : free;
                break;

            case def_kind::range:
                made.name  = unique_name(m_model.doc, "FIELD");
                made.name_children = true;
                made.lsb   = (free < 0) ? 0 : free;
                made.width = 1;
                break;

            case def_kind::field_value:
            {
                made.name = unique_name(m_model.doc, "VALUE");

                const std::vector<int> ranges = range_choices(m_model.doc);

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
                made.name = unique_name(m_model.doc, "VALUE");
                break;

            case def_kind::expression:
                made.name = unique_name(m_model.doc, "COMBINED");
                made.expr = "0";
                break;

            case def_kind::macro:
                made.name   = unique_name(m_model.doc, "MACRO");
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
        m_model.ui.selection.assign(1, added);
        m_model.ui.anchor = added;
        ++m_model.version;
    }

    return;
}

void
app::delete_rows()
{
    const std::vector<int> doomed = m_model.ui.selection;

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
         });

    m_model.ui.selection.clear();
    status(format_dec(doomed.size()) + " row(s) deleted", false);

    return;
}

void
app::duplicate_rows()
{
    const std::vector<int> originals = m_model.ui.selection;
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

    m_model.ui.selection = copies;
    ++m_model.version;

    return;
}

void
app::move_row_by(
    int _step
)
{
    const row* selected = inspected(m_model);

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
    const row* onto = find_row(m_model.doc, _onto);

    if ( (onto == nullptr) ||
         (_dragged == _onto) )
    {
        return;
    }

    int parent = _onto;
    int index  = -1;

    if (!can_contain(*onto))
    {
        const row* holder = parent_of(m_model.doc, _onto);

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

    return;
}

// --------------------------------------------------------------- rows: edit

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

    if (find_row(m_model.doc, id) == nullptr)
    {
        return;
    }

    const long        number = payload.as_long("value", 0);
    const bool        flag   = payload.as_bool("value", false);
    const std::string text   = payload.as_string("value", "");
    const std::vector<int> ranges = range_choices(m_model.doc);

    edit("row." + field + ":" + format_dec(static_cast<u64>(id)),
         [&](sheet& _sheet)
         {
             row& r = *find_row(_sheet, id);

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
                     const resolved_row* field_row = m_model.res.find(r.field);
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
                     static_cast<float>(payload.as_double("r", 1.0)),
                     static_cast<float>(payload.as_double("g", 1.0)),
                     static_cast<float>(payload.as_double("b", 1.0)),
                     static_cast<float>(payload.as_double("a", 1.0)));
             }
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
    const row*          source   = find_row(m_model.doc, _id);
    const resolved_row* resolved = m_model.res.find(_id);

    if ( (source == nullptr)                  ||
         (resolved == nullptr)                ||
         (source->kind != row_kind::symbol)   ||
         (_bit < 0)                           ||
         (_bit >= m_model.doc.width) )
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
    const row* source = find_row(m_model.doc, _id);
    const int  low    = std::min(_low, _high);
    const int  high   = std::max(_low, _high);

    if ( (source == nullptr) ||
         (low < 0)           ||
         (high >= m_model.doc.width) )
    {
        return;
    }

    const resolved_row* resolved = m_model.res.find(_id);
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
        const row* selected = inspected(m_model);
        int        lsb      = 0;
        int        msb      = std::min(7, m_model.doc.width - 1);

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

void
app::open_file()
{
    std::string text;
    std::string error;

    if (!read_file(m_model.ui.path, text, error))
    {
        status(error, true);

        return;
    }

    // a header, or any other code, is imported as a new sheet
    if (!looks_like_sheet(text))
    {
        import_code_text(m_model.ui.path, text);

        return;
    }

    import_json_text(m_model.ui.path, text);

    return;
}

void
app::save_file()
{
    std::string error;

    if (!write_file(m_model.ui.path, save_sheet(m_model.doc), error))
    {
        status(error, true);

        return;
    }

    m_model.ui.dirty = false;
    m_model.steps.seal();
    status("saved " + m_model.ui.path, false);

    return;
}

void
app::export_header()
{
    const std::string path   = directory_of(m_model.ui.path) +
                               m_model.doc.gen.file_name;
    const generated   header = generate(m_model.doc,
                                        m_model.res,
                                        m_model.today);
    std::string       error;

    if (!write_file(path, header.text, error))
    {
        status(error, true);

        return;
    }

    status("wrote " + path + ": " + format_dec(
               static_cast<u64>(header.macros)) + " macros" +
           ((header.skipped > 0)
                ? ", " + format_dec(static_cast<u64>(header.skipped)) +
                      " left out for errors"
                : std::string()),
           header.skipped > 0);

    return;
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
    const std::string stem = json_stem(m_model.ui.path);
    const std::string path = directory_of(m_model.ui.path) +
                             (stem.empty() ? m_model.doc.title : stem) +
                             (_everything ? ".export.json"
                                          : ".bitfield.json");
    std::string       error;
    const std::string text =
        _everything ? write_export(m_model.doc,
                                   m_model.res,
                                   generate(m_model.doc, m_model.res,
                                            m_model.today))
                    : write_bitfield(m_model.doc);

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
        load_sheet_into(std::move(read.result), _path);
        status("opened " + _path, false);

        return;
    }

    const std::string base  = _path.empty() ? read.result.title
                                            : json_stem(_path);
    const std::string where = directory_of(_path.empty() ? m_model.ui.path
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

    load_sheet_into(std::move(read.result), where + base + ".bitwisedom.json");

    m_model.ui.dirty        = true;
    m_model.ui.import_name  = from;
    m_model.ui.import_notes = read.notes;
    m_model.ui.fit_columns  = true;

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
    ui_state& ui = m_model.ui;

    ui.menu_kind  = _kind;
    ui.menu_where = _where;
    ui.menu_open  = true;

    if (_kind == "row")
    {
        const int id = static_cast<int>(_payload.as_long("value", 0));

        if (find_row(m_model.doc, id) == nullptr)
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
        m_model.preview_valid = false;
    }
    else if (_kind == "segment")
    {
        ui.menu_segment = static_cast<int>(_payload.as_long("value", -1));
        ui.menu_lsb     = static_cast<int>(_payload.as_long("lsb", 0));
        ui.menu_msb     = static_cast<int>(_payload.as_long("msb", 0));
    }

    ++m_model.version;

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
    const row* found = find_row(m_model.doc, _id);

    if ( (found == nullptr) ||
         (_id == 0) )
    {
        return;
    }

    // the groups around it, opened
    bool closed = false;

    for (row* parent = parent_of(m_model.doc, _id);
         ( (parent != nullptr) && (parent->id != 0) );
         parent = parent_of(m_model.doc, parent->id))
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

    ui_state& ui = m_model.ui;

    ui.selection.assign(1, _id);
    ui.anchor           = _id;
    ui.renaming         = _id;
    ui.renaming_segment = -1;
    ui.rename_text      = find_row(m_model.doc, _id)->name;
    ui.rename_where     = _where;
    ui.rename_focus     = true;
    ++m_model.version;

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
    m_model.ui.renaming = 0;
    ++m_model.version;

    const row* found = find_row(m_model.doc, _id);

    if ( (found == nullptr) ||
         (found->name == _text) )
    {
        return;
    }

    const std::string before = found->name;
    sheet             trial  = m_model.doc;
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

        index = static_cast<long>(m_model.doc.segments.size()) - 1;
    }

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= m_model.doc.segments.size()) )
    {
        return;
    }

    ui_state& ui = m_model.ui;

    ui.renaming         = 0;
    ui.renaming_segment = static_cast<int>(index);
    ui.rename_text      =
        m_model.doc.segments[static_cast<std::size_t>(index)].name;
    ui.rename_where     = "grid";
    ui.rename_focus     = true;
    ++m_model.version;

    return;
}

void
app::finish_segment_rename(
    const std::string& _text
)
{
    const int index = m_model.ui.renaming_segment;

    m_model.ui.renaming_segment = -1;
    ++m_model.version;

    if ( (index < 0) ||
         (static_cast<std::size_t>(index) >= m_model.doc.segments.size()) ||
         (m_model.doc.segments[static_cast<std::size_t>(index)].name ==
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
        for (const row& child : m_model.doc.root.children)
        {
            rows.push_back(child.id);
        }
    }
    else if (is_selected(m_model, _id))
    {
        rows = m_model.ui.selection;
    }
    else
    {
        rows.push_back(_id);
    }

    const generated code = generate_code(m_model.doc, m_model.res, rows);

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

    options.level = m_model.doc.gen.level;
    options.model = m_model.doc.gen.model;
    options.title = m_model.doc.title;

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

    m_model.ui.selection    = result.ids;
    m_model.ui.import_name  = "the clipboard";
    m_model.ui.import_notes = in.notes;
    m_model.ui.import_notes.insert(m_model.ui.import_notes.end(),
                                   result.notes.begin(),
                                   result.notes.end());
    ++m_model.version;

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
    options.level     = ( (cpp_file) && (!is_cpp(m_model.doc.gen.level)) )
                            ? standard::cpp17
                            : m_model.doc.gen.level;
    options.model     = m_model.doc.gen.model;
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

    load_sheet_into(std::move(in.result), sheet_path);

    m_model.ui.dirty           = true;
    m_model.ui.import_name     = _path.empty() ? std::string("the clipboard")
                                               : name;
    m_model.ui.import_notes    = in.notes;
    m_model.ui.fit_columns     = true;

    status("imported " + m_model.ui.import_name + ": " +
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
    // more changed than it was
    const bool arriving = _payload.as_bool("arriving", false);
    const bool dirty    = m_model.ui.dirty;

    if (width <= 0.0f)
    {
        return;
    }

    quietly([&column, width](sheet& _sheet)
            {
                if (column == "value")
                {
                    _sheet.grid.value_width =
                        std::max(within_setting("grid.value_width", width),
                                 VALUE_FLOOR);
                }
                else if (column == "cells")
                {
                    _sheet.grid.cell_width =
                        within_setting("grid.cell_width", width);
                }
                else
                {
                    _sheet.grid.name_width =
                        within_setting("grid.name_width", width);
                }
            });

    if (arriving)
    {
        m_model.ui.dirty = dirty;
    }

    return;
}

// -------------------------------------------------------------- operations

void
app::apply_operation()
{
    const op_kind op   = static_cast<op_kind>(
        std::clamp(m_model.ui.op, 0, OP_COUNT - 1));
    const op_info info = describe(op);
    wide          operand;

    if ( (!sheet_number(m_model.ui.operand, m_model.doc.width, operand)) &&
         (info.operand[0] != '\0') )
    {
        status("the operand is not a number: " +
                   parse_wide(m_model.ui.operand).error,
               true);

        return;
    }

    const std::vector<int> targets = op_targets(m_model);
    const bool             probe   = m_model.ui.op_probe;
    const resolution       before  = m_model.res;
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
