/*******************************************************************************
* bitwisedom [app]                                                  settings.hpp
*
* The settings registries: every sheet-wide setting, and every one of the
* window's own, by path.
*   A setting is a path -- "grid.minor.thickness", "gen.level" -- with a
* label, a kind, its bounds or choices, and how to read and write it on a
* sheet. The settings panels are built from this table, and the one "set"
* event they post is applied through it, so a setting is declared once and
* is then both drawn and editable, with nothing to keep in step.
*   The window's own settings -- the user's, which no sheet keeps -- are a
* second table of the same shape, by paths under "environment.": the theme,
* how a row is added, the colour a new row takes, whether the window opens
* where it was left. They are read from and written to the shell, and the
* same "set" event edits them.
*   A settings file holds both: the window's settings, and a sheet's that
* are preferences rather than its content -- how it is drawn and written,
* not its title, its width or its names. Exported from one sheet and
* imported into another, it makes the second look and write as the first.
*   Values travel as uxoxo payloads: a flag as a bool, an integer or a
* choice as a long, a real as a double, text as a string, and a colour as
* the four reals "r", "g", "b", "a" a color_field reports.
*
* path:      /src/djinterp/tools/bitwisedom/app/settings.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.18
*******************************************************************************/

#ifndef BITWISEDOM_APP_SETTINGS_HPP
#define BITWISEDOM_APP_SETTINGS_HPP 1

// std
#include <functional>  // std::function
#include <string>      // std::string
#include <vector>      // std::vector
// uxoxo
#include <uxoxo/element.hpp>  // option_set
// bitwisedom
#include <djinterp/tools/bitwisedom/sheet.hpp>  // sheet, rgba
#include "./shell.hpp"                          // shell_state


namespace bitwisedom
{
namespace gui
{

// setting_kind
//   enum: how a setting is edited.
enum class setting_kind
{
    flag,
    integer,
    real,
    colour,
    choice,
    text
};

// setting
//   struct: one sheet-wide setting. `read` gives a flag, integer or choice
// as a long, a real as its value, a colour as its packed bits; `write`
// takes the payload an editing element posted.
struct setting
{
    std::string                               path;
    std::string                               label;
    setting_kind                              kind = setting_kind::flag;
    double                                    low  = 0.0;
    double                                    high = 1.0;
    std::vector<std::string>                  choices;
    std::string                               help;
    std::function<double(const sheet&)>       read;
    std::function<std::string(const sheet&)>  read_text;
    std::function<void(sheet&,
                       const uxoxo::option_set&)> write;
};

// NAMES_BY_VALUE
//   constant: the first choice of the "grid.names_by" setting that lists
// the symbols column's rows by value -- one choice for each value_form, in
// its order. The two before it list them by symbol name and by name.
const int NAMES_BY_VALUE = 2;

// registry -- the table, and one entry by path (nullptr when unknown)
const std::vector<setting>&  settings();
const setting*               find_setting(const std::string& _path);

// env_setting
//   struct: one of the window's own settings, by a path under
// "environment.": read from and written to the shell as a sheet's setting
// is to a sheet. A text setting has none here.
struct env_setting
{
    std::string                                     path;
    std::string                                     label;
    setting_kind                                    kind = setting_kind::flag;
    double                                          low  = 0.0;
    double                                          high = 1.0;
    std::vector<std::string>                        choices;
    std::string                                     help;
    std::function<double(const shell_state&)>       read;
    std::function<void(shell_state&,
                       const uxoxo::option_set&)>   write;
};

// ENVIRONMENT
//   constant: the first part of every path of the window's own settings.
const char* const ENVIRONMENT = "environment.";

// ADD_KINDS
//   constant: the kinds a row may be added as, in the order the add menus
// list them and "environment.add_kind" numbers them: a bit, a range, a
// value in a range, a literal, an expression, a macro, and a group.
const int ADD_KINDS = 7;

// add_kind_name / add_kind_of
//   a kind to add, by its place among ADD_KINDS: its name, and the kind
// "edit.add" takes -- a def_kind's number, or -1 for a group.
D_NODISCARD const char*  add_kind_name(int _index);
D_NODISCARD int          add_kind_of(int _index);

// registry -- the window's own table, and one entry by path
const std::vector<env_setting>&  environment_settings();
const env_setting*               find_env_setting(const std::string& _path);

// is_environment
//   whether a path names one of the window's own settings.
D_NODISCARD bool  is_environment(const std::string& _path);

// is_preference
//   whether a sheet's setting is a preference a settings file carries --
// how the sheet is drawn and written -- rather than its content: its
// title, width, affixes and names are its own.
D_NODISCARD bool  is_preference(const setting& _setting);

// SETTINGS_FORMAT / SETTINGS_VERSION
//   constants: what a settings file calls itself, and the newest version
// this build reads.
const char* const SETTINGS_FORMAT  = "bitwisedom.settings";
const int         SETTINGS_VERSION = 1;

// setting_value
//   struct: one setting a settings file holds: its path, and the payload a
// "set" of it posts.
struct setting_value
{
    std::string       path;
    uxoxo::option_set payload;
};

/**
 * @brief The window's own settings, and a sheet's preferences, as a
 *        settings file's text.
 *
 * @param[in] _shell  the window, whose own settings are written.
 * @param[in] _sheet  the sheet whose preferences are written.
 * @return the file's text: JSON, its settings by path.
 */
D_NODISCARD std::string  write_settings(const shell_state& _shell,
                                        const sheet&       _sheet);

/**
 * @brief Reads a settings file's text into the "set"s that apply it.
 *
 * @param[in]  _text    the file's text.
 * @param[out] _values  each setting it holds that this build knows, the
 *                      window's first; a path it does not know, or a value
 *                      of the wrong kind, is passed over.
 * @param[out] _error   why the text is not a settings file, when it is not.
 * @return `true` if the text is a settings file.
 */
D_NODISCARD bool  read_settings(const std::string&          _text,
                                std::vector<setting_value>& _values,
                                std::string&                _error);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_SETTINGS_HPP
