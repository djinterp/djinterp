/*******************************************************************************
* bitwisedom [bitwisedom]                                           exchange.hpp
*
* A bitfield as JSON for other programs, and from them.
*   Three shapes of JSON are read, and two of them written:
*
*     bitwisedom.sheet     the window's own file (persist.hpp): every
*                          setting, to be opened as it was left
*     bitwisedom.bitfield  the bitfield alone, as a person would write it:
*                          its width, its affixes, and its rows, each in as
*                          few keys as say what it is -- {"name": "EN",
*                          "bit": 0}, {"name": "MODE", "bits": "4..6",
*                          "values": {"FAST": 2}}. An object that names no
*                          format is read as one.
*     bitwisedom.export    everything: the sheet as its file holds it, its
*                          layout, every row as it resolves -- its macro
*                          name, its bits, its value and the integer C knows
*                          it by, its error -- the issues, and the header
*
*   A bitfield is read leniently and reported on strictly. A key a row does
* not have is passed over and noted, with where it was; a row with no
* definition, or with two, is an error that says where -- rows[1].rows[3]
* (PARITY) -- and nothing is made of the file. A value in a range names
* that range, by its own name or its macro name, and is placed once every
* row has been read.
*   docs/json.md describes the three, key by key.
*
* path:      /inc/djinterp/tools/bitwisedom/exchange.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHAPES
    ------
    1.  Formats
    2.  json_shape
    3.  json_read
2.  READING
    -------
3.  WRITING
    -------
*/

#ifndef BITWISEDOM_EXCHANGE_HPP
#define BITWISEDOM_EXCHANGE_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./codegen.hpp"  // generated
#include "./resolve.hpp"  // resolution
#include "./sheet.hpp"    // sheet


namespace bitwisedom
{


//==============================================================================
// 1.  SHAPES
//==============================================================================


// 1.1    Formats
//------------------------------------------------------------------------------
// BITFIELD_FORMAT / BITFIELD_VERSION, EXPORT_FORMAT / EXPORT_VERSION
//   constants: what a bitfield and an export call themselves, and the newest
// version of each this build reads.
const char* const BITFIELD_FORMAT  = "bitwisedom.bitfield";
const int         BITFIELD_VERSION = 1;
const char* const EXPORT_FORMAT    = "bitwisedom.export";
const int         EXPORT_VERSION   = 1;

// 1.2    json_shape
//------------------------------------------------------------------------------
// json_shape
//   enum: which of the three a JSON text was.
enum class json_shape : int
{
    sheet = 0,
    bitfield,
    everything
};

// 1.3    json_read
//------------------------------------------------------------------------------
// json_read
//   struct: what came of reading a JSON text as a sheet. `error` says why,
// and where, when `ok` is false; `notes` what was passed over when it is
// true.
struct json_read
{
    bool                     ok    = false;
    json_shape               shape = json_shape::sheet;
    sheet                    result;
    std::string              error;
    std::vector<std::string> notes;
};


//==============================================================================
// 2.  READING
//==============================================================================


/**
 * @brief Reads a sheet from JSON of any of the three shapes.
 *
 * @param[in] _text  the JSON: a sheet file, an export (its sheet is read),
 *                   or a bitfield.
 * @return the sheet and the shape it came in, or why there is none.
 */
D_NODISCARD json_read  read_json_sheet(std::string_view _text);


//==============================================================================
// 3.  WRITING
//==============================================================================


/**
 * @brief Writes a sheet as a bitfield.
 *
 * @note The definitions only, each key left out where the default says
 *       it: the window's settings -- the grid, its colours, its widths --
 *       stay in the sheet file. The generator settings that differ from a
 *       new sheet's are kept. Read back, it writes the same header.
 *
 * @param[in] _sheet  the sheet.
 * @return the JSON text.
 */
D_NODISCARD std::string  write_bitfield(const sheet& _sheet);

/**
 * @brief Writes everything about a sheet.
 *
 * @param[in] _sheet       the sheet, which the export holds whole.
 * @param[in] _resolution  its resolution: every row's bits, value, macro
 *                         name and error, and the issues.
 * @param[in] _header      its header, which the export holds with its
 *                         notes.
 * @return the JSON text.
 */
D_NODISCARD std::string  write_export(const sheet&      _sheet,
                                      const resolution& _resolution,
                                      const generated&  _header);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_EXCHANGE_HPP
