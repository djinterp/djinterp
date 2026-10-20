/*******************************************************************************
* bitwisedom [bitwisedom]                                            codegen.hpp
*
* The C/C++ header generator: a sheet as macros.
*   Every symbol becomes an object-like macro -- or, for C++, a constant --
* named by the affix cascade and valued by its definition, written in the
* radix and with the integer suffix the cascade gives it. An expression
* keeps its form with every name replaced by a macro name; a macro row is
* written as its author wrote it, with {name} placeholders replaced the same
* way. Beside each symbol go the macro templates that apply to its kind.
*   STYLES. In the djinterp style the header follows the house style guide:
* the banner at its fixed geometry, a table of contents, numbered sections
* for groups, a two-line brief over every macro, aligned values and an
* include guard derived from the file name. The plain style writes the same
* macros with one comment line apiece.
*   LEVELS. The language level decides what may be written: a binary literal
* needs C23 or C++14, `ull` needs C99 or C++11, `inline constexpr` C++17. A
* setting the level cannot honour is replaced by the nearest it can, and a
* note says so.
*
* path:      /inc/djinterp/tools/bitwisedom/codegen.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RESULTS
    -------
    1.  generated
2.  OPERATIONS
    ----------
    1.  generate
    2.  Literals and names
*/

#ifndef BITWISEDOM_CODEGEN_HPP
#define BITWISEDOM_CODEGEN_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./resolve.hpp"  // resolution
#include "./sheet.hpp"    // sheet, gen_settings


namespace bitwisedom
{


//==============================================================================
// 1.  RESULTS
//==============================================================================


// 1.1    generated
//------------------------------------------------------------------------------
// 1.1.1
// generated
//   struct: a generated header, and what the generator had to say about it.
// `skipped` counts symbols left out because they are in error.
struct generated
{
    std::string              text;
    std::vector<std::string> notes;
    int                      macros  = 0;
    int                      skipped = 0;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    generate
//------------------------------------------------------------------------------
/**
 * @brief Writes a sheet as a C or C++ header.
 *
 * @param[in] _sheet       the sheet; its gen settings say how.
 * @param[in] _resolution  its resolution.
 * @param[in] _date        the banner's date, as YYYY.MM.DD.
 * @return the header's text and the generator's notes. Symbols in error,
 *         and rows not marked for emission, are left out.
 */
D_NODISCARD generated generate(const sheet&       _sheet,
                               const resolution&  _resolution,
                               const std::string& _date);

/**
 * @brief Writes some of a sheet's rows as code to paste elsewhere.
 *
 * @note The rows' definitions, briefs and templates, under their groups'
 *       names, as the header writes them -- without the banner, the table
 *       of contents, the guard, or the section numbers, which belong to a
 *       whole header.
 *
 * @param[in] _sheet       the sheet.
 * @param[in] _resolution  its resolution.
 * @param[in] _rows        the rows; a group stands for everything in it.
 * @return the code and the generator's notes.
 */
D_NODISCARD generated generate_code(const sheet&            _sheet,
                                    const resolution&       _resolution,
                                    const std::vector<int>& _rows);

// 2.2    Literals and names
//------------------------------------------------------------------------------
// levels -- what a language level may spell
D_NODISCARD const char*  standard_name(standard _level);
D_NODISCARD bool         is_cpp(standard _level);
D_NODISCARD bool         has_binary_literals(standard _level);
D_NODISCARD bool         has_long_long(standard _level);

/**
 * @brief Writes one number as a C integer literal.
 *
 * @param[in] _value   the bits.
 * @param[in] _width   the sheet's width, for padding.
 * @param[in] _radix   the base; shift is written by the caller, and here
 *                     means hexadecimal.
 * @param[in] _suffix  the integer suffix.
 * @param[in] _sheet   the sheet, for the case and padding settings.
 * @param[in] _pad     whether to pad to the width's digits.
 * @param[in] _digits  the fewest digits to write, prefix aside; 0 for no
 *                     fewer than the value needs.
 * @return the literal, prefix and suffix included.
 */
D_NODISCARD std::string  format_literal(u64          _value,
                                        int          _width,
                                        radix        _radix,
                                        int_suffix   _suffix,
                                        const sheet& _sheet,
                                        bool         _pad,
                                        int          _digits = 0);
D_NODISCARD std::string  derive_guard(const std::string& _file_name);

/**
 * @brief The prefix of a wide sheet's array macros.
 *
 * @note A sheet wider than an integer is written as an array of words, and
 *       its header defines what works on such an array: the prefix with
 *       WORD_BITS, WORDS, WORD(n), MASK(n), TEST(a, n), SET(a, n) and
 *       CLEAR(a, n). The prefix is the sheet's own, or, when it has none,
 *       its title made a name; either way it ends in an underscore.
 *
 * @param[in] _sheet  the sheet.
 * @return the prefix.
 */
D_NODISCARD std::string  bitset_prefix(const sheet& _sheet);

/**
 * @brief Emit one elementary #define per resolvable symbol.
 *        This is the initial, deliberately plain .h/.hpp export.
 */
D_NODISCARD std::string generate_simple_defines(const sheet& _sheet,
                                                const resolution& _resolved);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CODEGEN_HPP
