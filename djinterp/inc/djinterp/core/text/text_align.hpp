/*******************************************************************************
* djinterp [text]                                                text_align.hpp
*
*   Text alignment definition, used for any text justification or alignment
* purpose.
*
*
* path:      /inc/djinterp/core/text/text_align.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                        created: 2026.04.16
*******************************************************************************/

#ifndef  DJINTERP_TEXT_ALIGN_
#define  DJINTERP_TEXT_ALIGN_ 1

// std
#include <cstdint>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// text_alignment
//   enum: horizontal text alignment for a column, cell, or label.
enum class text_alignment : std::uint8_t
{
    left,
    center,
    right
};


NS_END  // djinterp


#endif  // DJINTERP_TEXT_ALIGN_