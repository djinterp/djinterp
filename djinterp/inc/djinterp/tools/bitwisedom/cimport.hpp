/*******************************************************************************
* bitwisedom [bitwisedom]                                            cimport.hpp
*
* C and C++ code read into a sheet: its #defines become rows.
*   A header -- or any text with #defines in it, a snippet pasted from one --
* is read as a preprocessor reads it, by djinterp's lexer at the sheet's
* language and level, its conditionals followed the way the define
* environment follows them (cenv.hpp). Each #define in a branch taken
* becomes a row:
*
*     a literal      0x0A00, 42u, (-3)       a literal row
*     a one-bit run  (1u << 3)               a bit row
*     a mask         (0xFu << 4)             a range row
*     anything else  D_A | D_BIT(3), F(x)    a macro row, verbatim
*
* but a definition becomes a literal, bit or range row only when the
* generator writes it back exactly as it was written -- digits, case,
* suffix and all; otherwise it is kept as a macro, and is written as it
* was. A body over several lines keeps its lines and their indentation.
*   The comment directly above a definition becomes its note, a djinterp
* brief read as the brief it is; a rule of `=` begins a group, titled by
* the heading between it and the next rule when there is one, and named
* for what its symbols' names share when there is not. An include guard
* and `#pragma once` become the generator's settings, and a djinterp
* banner its author. Declarations and code are not definitions, and are
* left out, and said to be.
*
* path:      /inc/djinterp/tools/bitwisedom/cimport.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IMPORTING
    ---------
    1.  import_options
    2.  imported
    3.  import_code
2.  MERGING
    -------
    1.  merged
    2.  merge_rows
*/

#ifndef BITWISEDOM_CIMPORT_HPP
#define BITWISEDOM_CIMPORT_HPP 1

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
#include "./sheet.hpp"  // sheet, standard, data_model


namespace bitwisedom
{


//==============================================================================
// 1.  IMPORTING
//==============================================================================


// 1.1    import_options
//------------------------------------------------------------------------------
// 1.1.1
// import_options
//   struct: how to read the code, and what to call the sheet it becomes.
struct import_options
{
    standard    level = standard::c99;
    data_model  model = host_data_model();
    std::string title;       // empty: "imported"
    std::string file_name;   // the header the sheet writes; empty: title.h
};

// 1.2    imported
//------------------------------------------------------------------------------
// 1.2.1
// imported
//   struct: the sheet the code became, and what became of the code: how
// many definitions were read, as what, and the notes -- each with its line
// -- on what was assumed, skipped or wrong. `ok` is false only when the
// text has no definition at all.
struct imported
{
    sheet                    result;
    std::vector<std::string> notes;
    int                      definitions = 0;
    int                      literals    = 0;
    int                      allocations = 0;   // bit and range rows
    int                      macros      = 0;
    int                      groups      = 0;
    int                      skipped     = 0;   // lines of code left out
    bool                     ok          = false;
};

// 1.3    import_code
//------------------------------------------------------------------------------
/**
 * @brief Reads code's #defines into a new sheet.
 *
 * @param[in] _text     the code: a header, or any part of one.
 * @param[in] _options  the language to read it at, and the sheet's names.
 * @return the sheet -- 32 bits wide, or 64 when a value needs more -- and
 *         the report.
 */
D_NODISCARD imported import_code(std::string_view      _text,
                                 const import_options& _options);

// sheets or code -- whether a file's text is a saved sheet (JSON) rather
// than code to import
D_NODISCARD bool looks_like_sheet(std::string_view _text);


//==============================================================================
// 2.  MERGING
//==============================================================================


// 2.1    merged
//------------------------------------------------------------------------------
// 2.1.1
// merged
//   struct: what a merge did: rows added, rows whose definitions were
// replaced, the ids of both, and notes.
struct merged
{
    int                      added   = 0;
    int                      updated = 0;
    std::vector<int>         ids;
    std::vector<std::string> notes;
};

// 2.2    merge_rows
//------------------------------------------------------------------------------
/**
 * @brief Merges an imported sheet's rows into another sheet.
 *
 * @note A symbol whose macro name the sheet already has replaces that
 *       symbol's definition, in place; the rest are added under _parent,
 *       their groups with them. A name is made local to where it lands --
 *       the prefixes and suffixes around it taken off -- so its macro name
 *       stays what it was.
 *
 * @param[in,out] _into    the sheet.
 * @param[in]     _parent  the group to add to; a symbol's id adds beside
 *                         it; 0 adds to the sheet.
 * @param[in]     _from    the imported sheet.
 * @post   Pointers and references into _into's rows are invalid.
 * @return what was added and replaced.
 */
merged merge_rows(sheet&       _into,
                  int          _parent,
                  const sheet& _from);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CIMPORT_HPP
