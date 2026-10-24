/*******************************************************************************
* bitwisedom [bitwisedom]                                               edit.hpp
*
* Edits as the window and the command line make them alike.
*   What a user does to a sheet -- adds a row, duplicates, removes or moves
* rows -- was the window's to know; it is the model's, so that `bitwisedom
* add` refuses what the Add node window refuses, in the same words, and a
* row added either way takes the same name and the same bits.
*   A NEW ROW. A row is named, when it is given no name, for its kind --
* FLAG, FIELD, VALUE, COMBINED, MACRO, Group -- made unique with _2, _3 and
* on; a bit or a range starts at the lowest bit its group leaves free, or,
* in a range with none free, at the range's own lowest bit, where the
* collision shows.
*   TRYING ONE. A row is tried in a copy of the sheet before it is added,
* or before a row's new fields replace its own: the copy is resolved, and a
* symbol whose macro name -- its own with the affixes around it -- is not a
* C identifier, or is another symbol's, is refused, with why. The copy is
* the sheet to keep when it is not.
*   SORTING. The order the window shows each group's rows in, by a key, is
* made the sheet's own -- the order its header follows.
*
* path:      /inc/djinterp/tools/bitwisedom/edit.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  A NEW ROW
    ---------
    1.  Names
         1.  unique_name
         2.  fresh_name
    2.  Places
         1.  range_choices
         2.  container_choices
         3.  place_bit
2.  TRYING A ROW
    ------------
    1.  trial
    2.  try_insert
    3.  try_change
3.  ROWS
    ----
    1.  duplicate_rows
    2.  remove_rows
    3.  move_by
    4.  sort_rows
*/

#ifndef BITWISEDOM_EDIT_HPP
#define BITWISEDOM_EDIT_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./resolve.hpp"  // resolution, issue
#include "./sheet.hpp"    // sheet, row


namespace bitwisedom
{


//==============================================================================
// 1.  A NEW ROW
//==============================================================================


// 1.1    Names
//------------------------------------------------------------------------------
// 1.1.1
// unique_name
//   function: `_base`, or `_base` with _2, _3 and on after it: the first that
// no row of the sheet has as its own name.
D_NODISCARD std::string  unique_name(const sheet&       _sheet,
                                     const std::string& _base);

// 1.1.2
// fresh_name
//   function: the name a new row takes when it is given none: Group for a
// group; FLAG, FIELD, VALUE, VALUE, COMBINED or MACRO for a symbol of each
// definition, in def_kind's order; made unique.
D_NODISCARD std::string  fresh_name(const sheet& _sheet,
                                    row_kind     _kind,
                                    def_kind     _def);

// 1.2    Places
//------------------------------------------------------------------------------
// 1.2.1
// range_choices
//   function: every range of the sheet, by id, in the tree's order: what a
// value may be placed in.
D_NODISCARD std::vector<int>  range_choices(const sheet& _sheet);

// 1.2.2
// container_choices
//   function: everything a row may be put into, by id: the sheet's own
// rows, 0, then every group and range, in the tree's order.
D_NODISCARD std::vector<int>  container_choices(const sheet& _sheet);

// 1.2.3
// place_bit
//   function: the bit a new bit or range starts at in a group or range: the
// lowest it leaves free; with none free, a range's own lowest bit, and 0
// in a group.
D_NODISCARD int  place_bit(const sheet& _sheet,
                           int          _parent);


//==============================================================================
// 2.  TRYING A ROW
//==============================================================================


// 2.1.1
// trial
//   struct: a row tried in a copy of a sheet: the copy with the row in it,
// the row's id there -- 0 when it could not be put there; a changed row's
// own -- what the copy resolves to, the issues the copy finds with the
// row, and why the row is refused, empty when it is not.
struct trial
{
    sheet              result;
    int                added = 0;
    resolution         named;
    std::vector<issue> issues;
    std::string        refused;
};

// 2.1.2
// try_insert
/**
 * @brief Tries a row in a copy of a sheet.
 *
 * @note The row goes where insert_row puts it. It is refused when there is
 *       nowhere to put it -- `_parent` names no row, or a row that holds
 *       none -- and, a symbol, when its macro name is not a C identifier or
 *       is another symbol's. Its issues -- an overlap, a value outside its
 *       range -- are reported, not refused: the window and the command line
 *       add it, and say so.
 *
 * @param[in] _sheet   the sheet, which is not changed.
 * @param[in] _parent  the group or range to put it in; 0 for the sheet's
 *                     own rows; a symbol's id puts it beside that symbol.
 * @param[in] _index   its place there; -1 puts it last.
 * @param[in] _row     the row; its id is replaced.
 * @return the copy, the row's id in it, the copy's resolution and the
 *         row's issues, and why it is refused.
 */
D_NODISCARD trial  try_insert(const sheet& _sheet,
                              int          _parent,
                              int          _index,
                              const row&   _row);

/**
 * @brief Tries a row in a copy of a sheet, resolved with the colours a
 *        window shows it in; as try_insert.
 *
 * @param[in] _sheet   the sheet.
 * @param[in] _parent  where to put it.
 * @param[in] _index   its place there.
 * @param[in] _row     the row.
 * @param[in] _look    the colours, as resolve(_sheet, _look) takes them.
 * @return as try_insert.
 */
D_NODISCARD trial  try_insert(const sheet&           _sheet,
                              int                    _parent,
                              int                    _index,
                              const row&             _row,
                              const colour_settings& _look);

// 2.1.3
// try_change
/**
 * @brief Tries a row's new fields in a copy of a sheet, in place of its
 *        own.
 *
 * @note The row keeps its id, its place and the rows in it; everything
 *       else is `_row`'s. It is refused when the sheet has no row `_id`;
 *       when it would change kind, a group becoming a symbol or a symbol a
 *       group; when it holds rows and would hold none, a range made a bit;
 *       and, a symbol, as try_insert refuses one. Its issues are reported,
 *       not refused.
 *
 * @param[in] _sheet  the sheet, which is not changed.
 * @param[in] _id     the row.
 * @param[in] _row    its new fields; its id and its rows are not read.
 * @return the copy, `_id` as the row's id there -- 0 when the sheet has no
 *         such row -- the copy's resolution and the row's issues, and why
 *         the change is refused.
 */
D_NODISCARD trial  try_change(const sheet& _sheet,
                              int          _id,
                              const row&   _row);


//==============================================================================
// 3.  ROWS
//==============================================================================


// 3.1.1
// duplicate_rows
/**
 * @brief Duplicates rows, each copy just after its row, named as its row
 *        with _COPY after, made unique.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _ids    the rows; an id no row has is passed over.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return the copies' ids, in `_ids`' order.
 */
std::vector<int>  duplicate_rows(sheet&                  _sheet,
                                 const std::vector<int>& _ids);

// 3.1.2
// remove_rows
/**
 * @brief Removes rows, and everything in them; the display rules over them
 *        alone go with them, and the special rows among them stay where the
 *        rows were.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _ids    the rows; an id no row has is passed over.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return how many of them were removed.
 */
int               remove_rows(sheet&                  _sheet,
                              const std::vector<int>& _ids);

// 3.1.3
// move_by
/**
 * @brief Moves a row up or down among the rows of its group.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _id     the row.
 * @param[in]     _step   how far: -1 up, 1 down.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return `true` if it moved; `false` at the end it was moving past, or for
 *         a row the sheet does not have.
 */
bool              move_by(sheet& _sheet,
                          int    _id,
                          int    _step);

// 3.1.4
// sort_rows
/**
 * @brief Puts each group's rows in the order a key sorts them, as the
 *        window shows them sorted by it: the order shown made the sheet's
 *        own.
 *
 * @note As display_order sorts: groups first among their siblings, and
 *       rows the key cannot tell apart in the order they were. The grid's
 *       own sort goes back to the sheet's order, which is now this one, so
 *       the window shows what the header follows.
 *
 * @param[in,out] _sheet       the sheet.
 * @param[in]     _key         the key; sort_key::sheet changes nothing.
 * @param[in]     _descending  `true` for the key's order turned round.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return how many groups' rows changed their order: 0 when they were
 *         already in it.
 */
int               sort_rows(sheet&   _sheet,
                            sort_key _key,
                            bool     _descending);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_EDIT_HPP
