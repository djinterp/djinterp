/*******************************************************************************
* bitwisedom [bitwisedom]                                           settings.hpp
*
* A sheet's settings, by path.
*   A setting is a path -- "grid.minor.thickness", "gen.level" -- with a
* label, a kind, its bounds or its choices, and how it is read from a sheet
* and written to one. The window draws its settings panels from this table
* and applies what they post through it; the command line's `get` and `set`
* read and write the same paths, so a setting is declared once, and is then
* drawn, editable, and scriptable, with nothing to keep in step.
*   A CHOICE has two names for each of its values: the label the window
* lists -- "LSB 0", "byte 0 highest" -- and the word a file and a command
* line write -- lsb0, big -- which is the word the sheet file uses where it
* holds the same value.
*   AS TEXT, a setting's value is what `bitwisedom get` prints and `bitwisedom
* set` reads: a flag as true or false, an integer and a real as numbers, a
* choice as its word, a colour as #RRGGBBAA, text as it is. Nothing is read
* by locale, and a value outside a setting's bounds is refused rather than
* held to them, so that a script learns of its mistake.
*
* path:      /inc/djinterp/tools/bitwisedom/settings.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Kinds
         1.  setting_kind
    2.  Choices of note
         1.  NAMES_BY_VALUE
2.  THE REGISTRY
    ------------
    1.  sheet_setting
    2.  sheet_settings / find_sheet_setting
    3.  is_preference
3.  AS TEXT
    -------
    1.  setting_text
    2.  set_setting
    3.  setting_values
*/

#ifndef BITWISEDOM_SETTINGS_HPP
#define BITWISEDOM_SETTINGS_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <functional>  // std::function
#include <string>      // std::string
#include <vector>      // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./sheet.hpp"  // sheet, rgba


namespace bitwisedom
{


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Kinds
//------------------------------------------------------------------------------
// 1.1.1
// setting_kind
//   enum: what a setting holds, and so how it is edited and written.
enum class setting_kind
{
    flag,
    integer,
    real,
    colour,
    choice,
    text
};

// 1.2    Choices of note
//------------------------------------------------------------------------------
// 1.2.1
// NAMES_BY_VALUE
//   constant: the first choice of "grid.names_by" that lists the symbols
// column's rows by value -- one choice for each value_form, in its order.
// The two before it list them by symbol name and by name.
const int NAMES_BY_VALUE = 2;


//==============================================================================
// 2.  THE REGISTRY
//==============================================================================


// 2.1.1
// sheet_setting
//   struct: one sheet-wide setting. `read` gives a flag as 0 or 1, an
// integer as itself, a choice as its index, a real as its value and a
// colour as its packed bits; `write` takes the same, held to the setting's
// bounds. A text setting reads and writes through `read_text` and
// `write_text` instead. `choices` are the labels the window lists, and
// `words` the words for the same values, one each. `automatic` is true for
// a setting the theme and scheme choose while the sheet holds none of its
// own -- a colour while it is 0, a real while it is below its bounds, which
// `write` takes as -1 -- whose text is then `auto`.
struct sheet_setting
{
    std::string                                       path;
    std::string                                       label;
    setting_kind                                      kind = setting_kind::flag;
    double                                            low  = 0.0;
    double                                            high = 1.0;
    bool                                              automatic = false;
    std::vector<std::string>                          choices;
    std::vector<std::string>                          words;
    std::string                                       help;
    std::function<double(const sheet&)>               read;
    std::function<std::string(const sheet&)>          read_text;
    std::function<void(sheet&, double)>               write;
    std::function<void(sheet&, const std::string&)>   write_text;
};

// 2.1.2
// sheet_settings / find_sheet_setting
//   functions: the table, in the order the window's panels list it; and
// one entry by its path, nullptr when no setting has it.
D_NODISCARD const std::vector<sheet_setting>&  sheet_settings();
D_NODISCARD const sheet_setting*               find_sheet_setting(
                                                   const std::string& _path);

// 2.1.3
// is_preference
//   predicate: whether a setting is a preference -- how a sheet is drawn and
// written, which a settings file carries from sheet to sheet -- rather than
// its content: its title, width, affixes and names are its own.
D_NODISCARD bool  is_preference(const std::string& _path);


//==============================================================================
// 3.  AS TEXT
//==============================================================================


// 3.1.1
// setting_text
//   function: a setting's value as text: true or false, a number, a word,
// #RRGGBBAA, the text itself, or auto, the theme's and scheme's choice.
D_NODISCARD std::string  setting_text(const sheet_setting& _setting,
                                      const sheet&         _sheet);

// 3.1.2
// set_setting
/**
 * @brief Writes a setting's value, given as text.
 *
 * @note A flag takes true, false, yes, no, on, off, 1 or 0, in any case; an
 *       integer, decimal digits, or 0x, 0o or 0b and the digits of that
 *       base, `_` between digits; a real, a decimal number; a choice, one
 *       of its words; a colour, #RRGGBB or #RRGGBBAA; text, anything. A
 *       setting the theme and scheme may choose takes `auto` too, in any
 *       case, which gives the choice back to them.
 *
 * @param[in,out] _sheet    the sheet.
 * @param[in]     _setting  the setting.
 * @param[in]     _value    the value.
 * @param[out]    _error    why it was refused, when it was: in lower case,
 *                          no full stop, without the path.
 * @return `true` if the sheet now holds the value; `false`, the sheet as it
 *         was, for a value of the wrong form or outside the setting's
 *         bounds.
 */
D_NODISCARD bool  set_setting(sheet&               _sheet,
                              const sheet_setting& _setting,
                              const std::string&   _value,
                              std::string&         _error);

// 3.1.3
// setting_values
//   function: what values a setting takes, as help and a refusal say it:
// "true or false", "an integer from 2 to 32", "lsb0 or msb0", "a number
// from 0 to 1, or auto".
D_NODISCARD std::string  setting_values(const sheet_setting& _setting);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_SETTINGS_HPP
