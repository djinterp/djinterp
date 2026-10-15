/*******************************************************************************
* bitwisedom [bitwisedom]                                            resolve.hpp
*
* Resolving a sheet: every row's macro name, value, colour and issues.
*   A sheet says what each symbol is defined as; its resolution says what
* that comes to. Names are composed from the affix cascade, values computed
* -- expressions in dependency order, a cycle reported rather than followed
* -- and the sheet checked: names C would reject or reserves, names defined
* twice, bits outside the width, numbers an encoding cannot hold, and
* allocations that overlap within a group that asks for its bits to be
* exclusive.
*   The usage of each bit -- how many bit and range symbols claim it -- is
* the distribution the grid's usage row shows, and the one an operation's
* preview compares before and after.
*   A macro row is evaluated as C: the sheet's macros, its environment text
* and the standard headers' definitions form an environment (cenv.hpp), its
* other symbols are constants of the types their literals give them, and
* the macro's name is expanded and evaluated in it. Its type and expansion
* are kept, and what the evaluation found -- undefined behaviour above all
* -- is reported on the row.
*   A resolution is computed whole, from scratch, after every edit: sheets
* are small, and a resolution that is always fresh cannot go stale.
*
* path:      /inc/djinterp/tools/bitwisedom/resolve.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RESULTS
    -------
    1.  issue
    2.  resolved_row
    3.  resolution
2.  OPERATIONS
    ----------
    1.  resolve
    2.  Names
*/

#ifndef BITWISEDOM_RESOLVE_HPP
#define BITWISEDOM_RESOLVE_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <map>      // std::map
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./sheet.hpp"  // sheet, row, rgba


namespace bitwisedom
{


//==============================================================================
// 1.  RESULTS
//==============================================================================


// 1.1    issue
//------------------------------------------------------------------------------
// 1.1.1
// severity
//   enum: how much an issue matters. An error keeps a symbol out of the
// generated header; a warning and a note do not.
enum class severity : int
{
    note = 0,
    warning,
    error
};

// 1.1.2
// issue
//   struct: one thing the check found, and the row it is about.
struct issue
{
    severity    level = severity::note;
    int         row   = 0;
    std::string message;
};

// 1.2    resolved_row
//------------------------------------------------------------------------------
// 1.2.1
// resolved_row
//   struct: what one row comes to. `bits` is every bit the row sets, in any
// sheet -- what the grid shows; `value` is the row's value as an integer:
// in a sheet up to 64 bits wide, the same bits, and in a wider one, the
// low 64 of them. `span_lsb` and `span_width` are the bits a bit, range or
// field_value symbol lives in (a field_value's are its field's); -1 and 0
// for the others.
struct resolved_row
{
    int         id        = 0;
    int         parent    = 0;
    int         depth     = 0;
    row_kind    kind      = row_kind::symbol;
    def_kind    def       = def_kind::bit;
    std::string local;
    std::string full;
    std::string prefix;          // every prefix the name took, in order
    std::string suffix;          // every suffix the name took, in order
    bool        has_value = false;
    u64         value     = 0;
    wide        bits;
    bool        negative  = false;
    i64         number    = 0;   // a field_value's number, decoded back
    int         span_lsb  = -1;
    int         span_width = 0;
    int         field     = 0;   // a field_value's range row, if any
    bool        visible   = true;
    rgba        colour    = 0;
    radix       fmt       = radix::hex;
    int_suffix  suf       = int_suffix::u;
    wide        overlap;         // bits shared with an exclusive sibling
    int         children  = 0;
    std::string error;
    std::string ctype;           // a macro's value's C type
    std::string expansion;       // a macro's name, fully expanded
};

// 1.3    resolution
//------------------------------------------------------------------------------
// 1.3.1
// resolution
//   struct: a whole sheet resolved. `rows` is in the tree's depth-first
// order, root excluded; `usage` has an entry per bit of the sheet's width.
struct resolution
{
    int                        width = 0;
    std::vector<resolved_row>  rows;
    std::vector<issue>         issues;
    std::vector<int>           usage;
    wide                       allocated;
    wide                       collisions;
    int                        errors     = 0;
    int                        warnings   = 0;
    std::map<int, std::size_t> index;
    std::map<std::string, int> by_full;

    // find
    //   accessor: the resolved row for an id, or nullptr.
    D_NODISCARD const resolved_row* find(int _id) const
    {
        const auto found = index.find(_id);

        return (found == index.end()) ? nullptr
                                      : &rows[found->second];
    }
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    resolve
//------------------------------------------------------------------------------
/**
 * @brief Resolves a sheet.
 *
 * @param[in] _sheet  the sheet.
 * @return every row's name, value and colour, every issue found, and the
 *         usage of each bit. A row in error has no value and is named in
 *         an issue of severity error.
 */
D_NODISCARD resolution resolve(const sheet& _sheet);

/**
 * @brief Resolves a sheet as a window shows it.
 *
 * @param[in] _sheet  the sheet.
 * @param[in] _look   the colours it is shown with, every automatic one
 *                    filled in (make_look): the rows the sheet leaves
 *                    automatic take their colours from its palette,
 *                    saturation and brightness.
 * @return as resolve(_sheet).
 */
D_NODISCARD resolution resolve(const sheet&           _sheet,
                               const colour_settings& _look);

// 2.2    Names
//------------------------------------------------------------------------------
/**
 * @brief Finds the row a name means, as seen from one row.
 *
 * @note A macro name matches first. A local name then matches if one row
 *       has it, or one sibling of _from does; otherwise it is ambiguous.
 *
 * @param[in]  _resolution  the sheet's resolution.
 * @param[in]  _name        the name, local or macro.
 * @param[in]  _from        the row the name is written in.
 * @param[out] _error       why there is no row, when there is none.
 * @return the row's id, or 0.
 */
D_NODISCARD int find_name(const resolution&  _resolution,
                          const std::string& _name,
                          int                _from,
                          std::string&       _error);

// 2.3    A wide sheet's C
//------------------------------------------------------------------------------
// 2.3.1
// c_constant
//   struct: the integer C knows a row of a wide sheet by, if it knows it by
// one.
struct c_constant
{
    bool        ok       = false;
    u64         value    = 0;
    bool        negative = false;
    std::string error;
};

/**
 * @brief The integer a row of a sheet wider than an integer is in C.
 *
 * @note No integer holds such a sheet's values, so its symbols are what
 *       names a place in its arrays: a bit or a range is its lowest bit's
 *       number, a value the bits its field holds -- what the field's _GET
 *       reads, so a negative number is its field's pattern -- a literal
 *       its number, a macro the integer it evaluates to. An expression's
 *       bits, and a literal's past the 64th, are a set no integer holds,
 *       and have none.
 *
 * @param[in] _row  the resolved row.
 * @return the integer, or `ok == false` and why.
 */
D_NODISCARD c_constant wide_constant(const resolved_row& _row);

// identifiers -- C's rules, and the forms C and C++ reserve
D_NODISCARD bool is_identifier(const std::string& _name);
D_NODISCARD bool is_reserved(const std::string& _name);

/**
 * @brief A macro row's body as C: each {NAME} replaced by that symbol's
 *        macro name.
 *
 * @note A brace pair is a placeholder only around a name that is not one
 *       of the macro's parameters and names a symbol; any other is C's
 *       brace, and stays.
 *
 * @param[in]  _resolution  the sheet's resolution.
 * @param[in]  _macro       the macro row.
 * @param[out] _unknown     the placeholders' names that name no symbol;
 *                          may be nullptr.
 * @return the body.
 */
D_NODISCARD std::string macro_body(const resolution&         _resolution,
                                   const row&                _macro,
                                   std::vector<std::string>* _unknown);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_RESOLVE_HPP
