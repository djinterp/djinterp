/*******************************************************************************
* bitwisedom [app]                                                  settings.hpp
*
* The settings registry: every sheet-wide setting, by path.
*   A setting is a path -- "grid.minor.thickness", "gen.level" -- with a
* label, a kind, its bounds or choices, and how to read and write it on a
* sheet. The settings panels are built from this table, and the one "set"
* event they post is applied through it, so a setting is declared once and
* is then both drawn and editable, with nothing to keep in step.
*   Values travel as uxoxo payloads: a flag as a bool, an integer or a
* choice as a long, a real as a double, text as a string, and a colour as
* the four reals "r", "g", "b", "a" a color_field reports.
*
* path:      /src/djinterp/tools/bitwisedom/app/settings.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
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

// registry -- the table, and one entry by path (nullptr when unknown)
const std::vector<setting>&  settings();
const setting*               find_setting(const std::string& _path);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_SETTINGS_HPP
