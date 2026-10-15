/*******************************************************************************
* bitwisedom [bitwisedom]                                             rename.hpp
*
* Renaming a row, and everything that refers to it.
*   A symbol is referred to by name in three places: an expression's text,
* by its own name or its macro name; a macro's body, by its macro name, as
* C names it; and a macro's {NAME} placeholders, by either. A rename finds
* each reference -- through djinterp's lexer, so that a string, a comment or
* a macro's parameter of the same spelling is not one -- and rewrites it,
* so the sheet means after the rename what it meant before. A field_value's
* range is referred to by id, and needs nothing.
*   A group's name is in no macro name, so renaming one changes nothing
* else; a group's prefix or suffix would, and is not a rename.
*
* path:      /inc/djinterp/tools/bitwisedom/rename.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RENAMING
    --------
    1.  renamed
    2.  rename_row
*/

#ifndef BITWISEDOM_RENAME_HPP
#define BITWISEDOM_RENAME_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./sheet.hpp"  // sheet


namespace bitwisedom
{


//==============================================================================
// 1.  RENAMING
//==============================================================================


// 1.1    renamed
//------------------------------------------------------------------------------
// 1.1.1
// renamed
//   struct: what a rename did: whether it was made, why not when it was
// not, and how many references it rewrote, in which rows.
struct renamed
{
    bool             ok         = false;
    std::string      error;
    int              references = 0;
    std::vector<int> rows;
};

// 1.2    rename_row
//------------------------------------------------------------------------------
/**
 * @brief Renames a row, rewriting every reference to it.
 *
 * @note A symbol's new name must be a C identifier once the affixes around
 *       it are added, and no other symbol's; a group's must not be empty.
 *       The same name again is a rename that changes nothing.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _id     the row.
 * @param[in]     _name   its new name, its own: without the affixes.
 * @return what was done, or why nothing was.
 */
renamed rename_row(sheet&             _sheet,
                   int                _id,
                   const std::string& _name);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_RENAME_HPP
