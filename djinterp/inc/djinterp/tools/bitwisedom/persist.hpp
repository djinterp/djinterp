/*******************************************************************************
* bitwisedom [bitwisedom]                                            persist.hpp
*
* Saving and loading sheets, as JSON.
*   A sheet file is one JSON object: the sheet's own fields, its rows as a
* tree, its column segments, and its grid, colour and generator settings.
* Enumerations are written as words and colours as #RRGGBBAA, so the file
* reads as what it means; 64-bit values are written as 0x-prefixed strings,
* which no JSON reader rounds.
*   Loading is forgiving where it can be: a missing key keeps its default, so
* a file from an older version still opens. It is strict where it must be:
* a file that is not JSON, or not a sheet, or from a newer version, is
* refused with the reason.
*
* path:      /inc/djinterp/tools/bitwisedom/persist.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_PERSIST_HPP
#define BITWISEDOM_PERSIST_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>       // std::string
#include <string_view>  // std::string_view
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./json.hpp"   // json_value
#include "./sheet.hpp"  // sheet


namespace bitwisedom
{

// SHEET_FORMAT / SHEET_VERSION
//   constants: what a sheet file calls itself, and the newest version this
// build reads.
const char* const SHEET_FORMAT  = "bitwisedom.sheet";
const int         SHEET_VERSION = 1;

// text -- a sheet as file text, and back
D_NODISCARD std::string  save_sheet(const sheet& _sheet);

/**
 * @brief Reads a sheet from file text.
 *
 * @param[in]  _text   the file's text.
 * @param[out] _sheet  the sheet, when the text is one.
 * @param[out] _error  why it is not, when it is not.
 * @return `true` if _sheet now holds the file's sheet.
 */
D_NODISCARD bool  load_sheet(std::string_view _text,
                             sheet&           _sheet,
                             std::string&     _error);

// objects -- a sheet as the JSON object a sheet file holds, and back, for
// the formats that carry one (exchange.hpp)
D_NODISCARD json_value  sheet_object(const sheet& _sheet);
D_NODISCARD bool        sheet_from_object(const json_value& _object,
                                          sheet&            _sheet,
                                          std::string&      _error);

// generator settings -- as a sheet file holds them; reading keeps whatever
// the object leaves out
D_NODISCARD json_value  generator_object(const gen_settings& _gen);
void                    read_generator(const json_value* _object,
                                       gen_settings&     _gen);

// words -- each enumeration a file names, as the word it is written as;
// read_enum leaves the value as it was, and is false, for any other word
D_NODISCARD std::string  enum_word(encoding _value);
D_NODISCARD std::string  enum_word(radix _value);
D_NODISCARD std::string  enum_word(int_suffix _value);
D_NODISCARD std::string  enum_word(numbering _value);
D_NODISCARD std::string  enum_word(byte_order _value);
D_NODISCARD std::string  enum_word(def_kind _value);
D_NODISCARD bool  read_enum(const std::string& _word, encoding& _value);
D_NODISCARD bool  read_enum(const std::string& _word, radix& _value);
D_NODISCARD bool  read_enum(const std::string& _word, int_suffix& _value);
D_NODISCARD bool  read_enum(const std::string& _word, numbering& _value);
D_NODISCARD bool  read_enum(const std::string& _word, byte_order& _value);
D_NODISCARD bool  read_enum(const std::string& _word, def_kind& _value);

// files -- whole-file reads and writes, by a UTF-8 path; false, with a
// reason, on failure
D_NODISCARD bool  read_file(const std::string& _path,
                            std::string&       _text,
                            std::string&       _error);
D_NODISCARD bool  write_file(const std::string& _path,
                             const std::string& _text,
                             std::string&       _error);

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_PERSIST_HPP
