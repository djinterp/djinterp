/*******************************************************************************
* djinterp [core]                                                 text_align.hpp
*
*   Text alignment definition, used for any text justification or alignment
* purpose.
*
*
* path:      /inc/djinterp/core/text/text_align.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.16
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef  DJINTERP_TEXT_TEXT_ALIGN_HPP
#define  DJINTERP_TEXT_TEXT_ALIGN_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t


NS_DJINTERP


// text_alignment
//   enum: horizontal text alignment for a column, cell, label, or flow of
// wrapped lines. `justify` spreads inter-word space so both edges align and
// is meaningful only for multi-line, wrapped content (e.g. PDF paragraphs);
// single lines treat it as `left`.
enum class text_alignment : re_std::uint8_t
{
    left,
    center,
    right,
    justify
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_TEXT_ALIGN_HPP
