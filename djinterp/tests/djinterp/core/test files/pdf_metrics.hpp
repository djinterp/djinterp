/******************************************************************************
* djinterp [text]                                             pdf_metrics.hpp
*
* djinterp PDF text-metrics header:
*   Accurate glyph and string width measurement for the standard-14 PDF
* fonts.  This is the keystone of any real layout: center / right alignment,
* justification, word wrapping, and fitting text into a box all require the
* true rendered width of a string, which for proportional faces (Helvetica,
* Times) demands the actual Adobe Font Metrics rather than an average-advance
* approximation.
*
*   DATA:
*   The embedded width tables are the exact Adobe Core-14 advances expressed
* in 1000-em units, indexed by code point.  The Helvetica and Times families
* are tabulated under WinAnsiEncoding (matching the /Encoding the foundation
* emits); Symbol and ZapfDingbats use their own built-in encodings.  The
* Courier family is uniformly 600/1000 em and is computed rather than
* tabulated.  A glyph with no entry in a face contributes 0.
*
*   WIDTH MODEL:
*   The rendered width of a byte at point size S is:
*       width = advance_per_1000em[code] * S / 1000
*   String width is the sum of its bytes' widths.  Inputs are treated as
* single-byte (Latin-1 / WinAnsi) text, consistent with the foundation's
* WinAnsiEncoding output; multi-byte encodings are out of scope here.
*
*   WRAPPING:
*   wrap_to_width performs greedy word wrapping against a measured maximum
* width, breaking on spaces and hard-breaking any single word that exceeds
* the line on its own.  It is the metric-aware counterpart to the character
* cell wrapping used by fixed-pitch layout.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h / env_*.h via djinterp.hpp for version
* detection (D_NOEXCEPT, D_CONSTEXPR) and namespace macros.  No third-party
* dependency; the tables are compile-time constant data.
*
*
* TABLE OF CONTENTS
* =================
* I.    EMBEDDED WIDTH TABLES
* II.   GLYPH WIDTH
* III.  STRING WIDTH
* IV.   FITTING & TRUNCATION
* V.    WORD WRAPPING
*
*
* path:      /inc/djinterp/core/text/pdf/pdf_metrics.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_TEXT_PDF_METRICS_
#define DJINTERP_TEXT_PDF_METRICS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./pdf.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   EMBEDDED WIDTH TABLES                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // k_helvetica_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the helvetica standard-14 face.
    static const short k_helvetica_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584, 584, 556,
        1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
        667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278, 278, 278, 469, 556,
        333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556,
        556, 556, 333, 500, 278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584, 350,
        556, 350, 222, 556, 333, 1000, 556, 556, 333, 1000, 667, 333, 1000, 350, 611, 350,
        350, 222, 222, 333, 333, 350, 556, 1000, 333, 1000, 500, 333, 944, 350, 500, 667,
        278, 333, 556, 556, 556, 556, 260, 556, 333, 737, 370, 556, 584, 333, 737, 333,
        400, 584, 333, 333, 333, 556, 537, 278, 333, 333, 365, 556, 834, 834, 834, 611,
        667, 667, 667, 667, 667, 667, 1000, 722, 667, 667, 667, 667, 278, 278, 278, 278,
        722, 722, 778, 778, 778, 778, 778, 584, 778, 722, 722, 722, 722, 667, 667, 611,
        556, 556, 556, 556, 556, 556, 889, 500, 556, 556, 556, 556, 278, 278, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 584, 611, 556, 556, 556, 556, 500, 556, 500
    };

    // k_helvetica_bold_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the helvetica bold standard-14 face.
    static const short k_helvetica_bold_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        278, 333, 474, 556, 556, 889, 722, 238, 333, 333, 389, 584, 278, 333, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 333, 333, 584, 584, 584, 611,
        975, 722, 722, 722, 722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
        667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333, 278, 333, 584, 556,
        333, 556, 611, 556, 611, 556, 333, 611, 611, 278, 278, 556, 278, 889, 611, 611,
        611, 611, 389, 556, 333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584, 350,
        556, 350, 278, 556, 500, 1000, 556, 556, 333, 1000, 667, 333, 1000, 350, 611, 350,
        350, 278, 278, 500, 500, 350, 556, 1000, 333, 1000, 556, 333, 944, 350, 500, 667,
        278, 333, 556, 556, 556, 556, 280, 556, 333, 737, 370, 556, 584, 333, 737, 333,
        400, 584, 333, 333, 333, 611, 556, 278, 333, 333, 365, 556, 834, 834, 834, 611,
        722, 722, 722, 722, 722, 722, 1000, 722, 667, 667, 667, 667, 278, 278, 278, 278,
        722, 722, 778, 778, 778, 778, 778, 584, 778, 722, 722, 722, 722, 667, 667, 611,
        556, 556, 556, 556, 556, 556, 889, 556, 556, 556, 556, 556, 278, 278, 278, 278,
        611, 611, 611, 611, 611, 611, 611, 584, 611, 611, 611, 611, 611, 556, 611, 556
    };

    // k_helvetica_oblique_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the helvetica oblique standard-14 face.
    static const short k_helvetica_oblique_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584, 584, 556,
        1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
        667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278, 278, 278, 469, 556,
        333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556,
        556, 556, 333, 500, 278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584, 350,
        556, 350, 222, 556, 333, 1000, 556, 556, 333, 1000, 667, 333, 1000, 350, 611, 350,
        350, 222, 222, 333, 333, 350, 556, 1000, 333, 1000, 500, 333, 944, 350, 500, 667,
        278, 333, 556, 556, 556, 556, 260, 556, 333, 737, 370, 556, 584, 333, 737, 333,
        400, 584, 333, 333, 333, 556, 537, 278, 333, 333, 365, 556, 834, 834, 834, 611,
        667, 667, 667, 667, 667, 667, 1000, 722, 667, 667, 667, 667, 278, 278, 278, 278,
        722, 722, 778, 778, 778, 778, 778, 584, 778, 722, 722, 722, 722, 667, 667, 611,
        556, 556, 556, 556, 556, 556, 889, 500, 556, 556, 556, 556, 278, 278, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 584, 611, 556, 556, 556, 556, 500, 556, 500
    };

    // k_helvetica_bold_oblique_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the helvetica bold oblique standard-14 face.
    static const short k_helvetica_bold_oblique_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        278, 333, 474, 556, 556, 889, 722, 238, 333, 333, 389, 584, 278, 333, 278, 278,
        556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 333, 333, 584, 584, 584, 611,
        975, 722, 722, 722, 722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
        667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333, 278, 333, 584, 556,
        333, 556, 611, 556, 611, 556, 333, 611, 611, 278, 278, 556, 278, 889, 611, 611,
        611, 611, 389, 556, 333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584, 350,
        556, 350, 278, 556, 500, 1000, 556, 556, 333, 1000, 667, 333, 1000, 350, 611, 350,
        350, 278, 278, 500, 500, 350, 556, 1000, 333, 1000, 556, 333, 944, 350, 500, 667,
        278, 333, 556, 556, 556, 556, 280, 556, 333, 737, 370, 556, 584, 333, 737, 333,
        400, 584, 333, 333, 333, 611, 556, 278, 333, 333, 365, 556, 834, 834, 834, 611,
        722, 722, 722, 722, 722, 722, 1000, 722, 667, 667, 667, 667, 278, 278, 278, 278,
        722, 722, 778, 778, 778, 778, 778, 584, 778, 722, 722, 722, 722, 667, 667, 611,
        556, 556, 556, 556, 556, 556, 889, 556, 556, 556, 556, 556, 278, 278, 278, 278,
        611, 611, 611, 611, 611, 611, 611, 584, 611, 611, 611, 611, 611, 556, 611, 556
    };

    // k_times_roman_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the times roman standard-14 face.
    static const short k_times_roman_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        250, 333, 408, 500, 500, 833, 778, 180, 333, 333, 500, 564, 250, 333, 250, 278,
        500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 278, 278, 564, 564, 564, 444,
        921, 722, 667, 667, 722, 611, 556, 722, 722, 333, 389, 722, 611, 889, 722, 722,
        556, 722, 667, 556, 611, 722, 722, 944, 722, 722, 611, 333, 278, 333, 469, 500,
        333, 444, 500, 444, 500, 444, 333, 500, 500, 278, 278, 500, 278, 778, 500, 500,
        500, 500, 333, 389, 278, 500, 500, 722, 500, 500, 444, 480, 200, 480, 541, 350,
        500, 350, 333, 500, 444, 1000, 500, 500, 333, 1000, 556, 333, 889, 350, 611, 350,
        350, 333, 333, 444, 444, 350, 500, 1000, 333, 980, 389, 333, 722, 350, 444, 722,
        250, 333, 500, 500, 500, 500, 200, 500, 333, 760, 276, 500, 564, 333, 760, 333,
        400, 564, 300, 300, 333, 500, 453, 250, 333, 300, 310, 500, 750, 750, 750, 444,
        722, 722, 722, 722, 722, 722, 889, 667, 611, 611, 611, 611, 333, 333, 333, 333,
        722, 722, 722, 722, 722, 722, 722, 564, 722, 722, 722, 722, 722, 722, 556, 500,
        444, 444, 444, 444, 444, 444, 667, 444, 444, 444, 444, 444, 278, 278, 278, 278,
        500, 500, 500, 500, 500, 500, 500, 564, 500, 500, 500, 500, 500, 500, 500, 500
    };

    // k_times_bold_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the times bold standard-14 face.
    static const short k_times_bold_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        250, 333, 555, 500, 500, 1000, 833, 278, 333, 333, 500, 570, 250, 333, 250, 278,
        500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 333, 333, 570, 570, 570, 500,
        930, 722, 667, 722, 722, 667, 611, 778, 778, 389, 500, 778, 667, 944, 722, 778,
        611, 778, 722, 556, 667, 722, 722, 1000, 722, 722, 667, 333, 278, 333, 581, 500,
        333, 500, 556, 444, 556, 444, 333, 500, 556, 278, 333, 556, 278, 833, 556, 500,
        556, 556, 444, 389, 333, 556, 500, 722, 500, 500, 444, 394, 220, 394, 520, 350,
        500, 350, 333, 500, 500, 1000, 500, 500, 333, 1000, 556, 333, 1000, 350, 667, 350,
        350, 333, 333, 500, 500, 350, 500, 1000, 333, 1000, 389, 333, 722, 350, 444, 722,
        250, 333, 500, 500, 500, 500, 220, 500, 333, 747, 300, 500, 570, 333, 747, 333,
        400, 570, 300, 300, 333, 556, 540, 250, 333, 300, 330, 500, 750, 750, 750, 500,
        722, 722, 722, 722, 722, 722, 1000, 722, 667, 667, 667, 667, 389, 389, 389, 389,
        722, 722, 778, 778, 778, 778, 778, 570, 778, 722, 722, 722, 722, 722, 611, 556,
        500, 500, 500, 500, 500, 500, 722, 444, 444, 444, 444, 444, 278, 278, 278, 278,
        500, 556, 500, 500, 500, 500, 500, 570, 500, 556, 556, 556, 556, 500, 556, 500
    };

    // k_times_italic_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the times italic standard-14 face.
    static const short k_times_italic_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        250, 333, 420, 500, 500, 833, 778, 214, 333, 333, 500, 675, 250, 333, 250, 278,
        500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 333, 333, 675, 675, 675, 500,
        920, 611, 611, 667, 722, 611, 611, 722, 722, 333, 444, 667, 556, 833, 667, 722,
        611, 722, 611, 500, 556, 722, 611, 833, 611, 556, 556, 389, 278, 389, 422, 500,
        333, 500, 500, 444, 500, 444, 278, 500, 500, 278, 278, 444, 278, 722, 500, 500,
        500, 500, 389, 389, 278, 500, 444, 667, 444, 444, 389, 400, 275, 400, 541, 350,
        500, 350, 333, 500, 556, 889, 500, 500, 333, 1000, 500, 333, 944, 350, 556, 350,
        350, 333, 333, 556, 556, 350, 500, 889, 333, 980, 389, 333, 667, 350, 389, 556,
        250, 389, 500, 500, 500, 500, 275, 500, 333, 760, 276, 500, 675, 333, 760, 333,
        400, 675, 300, 300, 333, 500, 523, 250, 333, 300, 310, 500, 750, 750, 750, 500,
        611, 611, 611, 611, 611, 611, 889, 667, 611, 611, 611, 611, 333, 333, 333, 333,
        722, 667, 722, 722, 722, 722, 722, 675, 722, 722, 722, 722, 722, 556, 611, 500,
        500, 500, 500, 500, 500, 500, 667, 444, 444, 444, 444, 444, 278, 278, 278, 278,
        500, 500, 500, 500, 500, 500, 500, 675, 500, 500, 500, 500, 500, 444, 500, 444
    };

    // k_times_bold_italic_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the times bold italic standard-14 face.
    static const short k_times_bold_italic_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        250, 389, 555, 500, 500, 833, 778, 278, 333, 333, 500, 570, 250, 333, 250, 278,
        500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 333, 333, 570, 570, 570, 500,
        832, 667, 667, 667, 722, 667, 667, 722, 778, 389, 500, 667, 611, 889, 722, 722,
        611, 722, 667, 556, 611, 722, 667, 889, 667, 611, 611, 333, 278, 333, 570, 500,
        333, 500, 500, 444, 500, 444, 333, 500, 556, 278, 278, 500, 278, 778, 556, 500,
        500, 500, 389, 389, 278, 556, 444, 667, 500, 444, 389, 348, 220, 348, 570, 350,
        500, 350, 333, 500, 500, 1000, 500, 500, 333, 1000, 556, 333, 944, 350, 611, 350,
        350, 333, 333, 500, 500, 350, 500, 1000, 333, 1000, 389, 333, 722, 350, 389, 611,
        250, 389, 500, 500, 500, 500, 220, 500, 333, 747, 266, 500, 606, 333, 747, 333,
        400, 570, 300, 300, 333, 576, 500, 250, 333, 300, 300, 500, 750, 750, 750, 500,
        667, 667, 667, 667, 667, 667, 944, 667, 667, 667, 667, 667, 389, 389, 389, 389,
        722, 722, 722, 722, 722, 722, 722, 570, 722, 722, 722, 722, 722, 611, 611, 500,
        500, 500, 500, 500, 500, 500, 722, 444, 444, 444, 444, 444, 278, 278, 278, 278,
        500, 556, 500, 500, 500, 500, 500, 570, 500, 556, 556, 556, 556, 444, 500, 444
    };

    // k_symbol_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the symbol standard-14 face.
    static const short k_symbol_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        250, 333, 713, 500, 549, 833, 778, 439, 333, 333, 500, 549, 250, 549, 250, 278,
        500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 278, 278, 549, 549, 549, 444,
        549, 722, 667, 722, 612, 611, 763, 603, 722, 333, 631, 722, 686, 889, 722, 722,
        768, 741, 556, 592, 611, 690, 439, 768, 645, 795, 611, 333, 863, 333, 658, 500,
        500, 631, 549, 549, 494, 439, 521, 411, 603, 329, 603, 549, 549, 576, 521, 549,
        549, 521, 549, 603, 439, 576, 713, 686, 493, 686, 494, 480, 200, 480, 549,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        750, 620, 247, 549, 167, 713, 500, 753, 753, 753, 753, 1042, 987, 603, 987, 603,
        400, 549, 411, 549, 549, 713, 494, 460, 549, 549, 549, 549, 1000, 603, 1000, 658,
        823, 686, 795, 987, 768, 768, 823, 768, 768, 713, 713, 713, 713, 713, 713, 713,
        768, 713, 790, 790, 890, 823, 549, 250, 713, 603, 603, 1042, 987, 603, 987, 603,
        494, 329, 790, 790, 786, 713, 384, 384, 384, 384, 384, 384, 494, 494, 494, 494,
          0, 329, 274, 686, 686, 686, 384, 384, 384, 384, 384, 384, 494, 494, 494,   0
    };

    // k_zapf_dingbats_w
    //   data: WinAnsi-indexed glyph advances (per 1000 em) for
    // the zapf dingbats standard-14 face.
    static const short k_zapf_dingbats_w[256] =
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        278, 974, 961, 974, 980, 719, 789, 790, 791, 690, 960, 939, 549, 855, 911, 933,
        911, 945, 974, 755, 846, 762, 761, 571, 677, 763, 760, 759, 754, 494, 552, 537,
        577, 692, 786, 788, 788, 790, 793, 794, 816, 823, 789, 841, 823, 833, 816, 831,
        923, 744, 723, 749, 790, 792, 695, 776, 768, 792, 759, 707, 708, 682, 701, 826,
        815, 789, 789, 707, 687, 696, 689, 786, 787, 713, 791, 785, 791, 873, 761, 762,
        762, 759, 759, 892, 892, 788, 784, 438, 138, 277, 415, 392, 392, 668, 668,   0,
        390, 390, 317, 317, 276, 276, 509, 509, 410, 410, 234, 234, 334, 334,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0, 732, 544, 544, 910, 667, 760, 760, 776, 595, 694, 626, 788, 788, 788, 788,
        788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788,
        788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788, 788,
        788, 788, 788, 788, 894, 838, 1016, 458, 748, 924, 748, 918, 927, 928, 928, 834,
        873, 828, 924, 924, 917, 930, 931, 463, 883, 836, 836, 867, 867, 696, 696, 874,
          0, 874, 760, 946, 771, 865, 771, 888, 967, 888, 831, 873, 927, 970, 918,   0
    };

    // courier_width
    //   helper: the Courier family is monospaced at 600/1000 em for
    // every glyph, so its advance is computed rather than tabulated.
    inline short
    courier_width(
        unsigned char _code
    ) D_NOEXCEPT
    {
        // control range (< 0x20) has no printable glyph
        if (_code < 0x20)
        {
            return 0;
        }

        return 600;
    }

    // width_table_for
    //   helper: returns the 256-entry width table for a proportional
    // base font, or nullptr for the Courier family (computed) or an
    // unrecognized face.
    inline const short*
    width_table_for(
        pdf_base_font _font
    ) D_NOEXCEPT
    {
        switch (_font)
        {
            case pdf_base_font::helvetica:              { return k_helvetica_w; }
            case pdf_base_font::helvetica_bold:         { return k_helvetica_bold_w; }
            case pdf_base_font::helvetica_oblique:      { return k_helvetica_oblique_w; }
            case pdf_base_font::helvetica_bold_oblique: { return k_helvetica_bold_oblique_w; }
            case pdf_base_font::times_roman:            { return k_times_roman_w; }
            case pdf_base_font::times_bold:             { return k_times_bold_w; }
            case pdf_base_font::times_italic:           { return k_times_italic_w; }
            case pdf_base_font::times_bold_italic:      { return k_times_bold_italic_w; }
            case pdf_base_font::symbol:                 { return k_symbol_w; }
            case pdf_base_font::zapf_dingbats:          { return k_zapf_dingbats_w; }
            default:                                    { return nullptr; }
        }
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  GLYPH WIDTH                                          ///
///////////////////////////////////////////////////////////////////////////////

// glyph_advance_em
//   function: the advance width of a single byte in 1000-em units
// for the given base font.  Returns 0 for code points the face does
// not define.  Exact for every standard-14 face.
//
// Parameter(s):
//   _font: the standard-14 base font.
//   _code: the WinAnsi (or face-native, for Symbol / ZapfDingbats)
//          code point.
// Return:
//   The advance in 1000-em units.
inline int
glyph_advance_em(
    pdf_base_font _font,
    unsigned char _code
) D_NOEXCEPT
{
    const short* table = internal::width_table_for(_font);

    // Courier (and any unrecognized face) is monospaced at 600 em
    if (!table)
    {
        return internal::courier_width(_code);
    }

    return static_cast<int>(table[_code]);
}

// glyph_width
//   function: the rendered width in points of a single byte at the
// given font and size.
//
// Parameter(s):
//   _font: the standard-14 base font.
//   _size: point size.
//   _code: the code point.
// Return:
//   The rendered advance in points.
inline pdf_unit
glyph_width(
    pdf_base_font _font,
    pdf_unit      _size,
    unsigned char _code
) D_NOEXCEPT
{
    return ( static_cast<pdf_unit>(glyph_advance_em(_font, _code)) *
             _size / 1000.0 );
}


///////////////////////////////////////////////////////////////////////////////
///                III. STRING WIDTH                                         ///
///////////////////////////////////////////////////////////////////////////////

// text_width
//   function: the rendered width in points of a string at the given
// base font and size.  Bytes are treated as single-byte WinAnsi /
// face-native code points.
//
// Parameter(s):
//   _font: the standard-14 base font.
//   _size: point size.
//   _text: the string to measure.
// Return:
//   The summed advance in points.
inline pdf_unit
text_width(
    pdf_base_font      _font,
    pdf_unit           _size,
    const std::string& _text
) D_NOEXCEPT
{
    const short* table = internal::width_table_for(_font);
    long         total = 0;

    // sum per-glyph advances in em units, then scale once
    for (std::size_t i = 0; i < _text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(_text[i]);

        if (table)
        {
            total += static_cast<long>(table[c]);
        }
        else
        {
            total += static_cast<long>(internal::courier_width(c));
        }
    }

    return ( static_cast<pdf_unit>(total) * _size / 1000.0 );
}

// text_width
//   function: pdf_font overload measuring with the font's own size.
inline pdf_unit
text_width(
    const pdf_font&    _font,
    const std::string& _text
) D_NOEXCEPT
{
    return text_width(_font.family, _font.size, _text);
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  FITTING & TRUNCATION                                 ///
///////////////////////////////////////////////////////////////////////////////

// fit_char_count
//   function: the number of leading bytes of _text whose cumulative
// width does not exceed _max_width at the given font and size.
//
// Parameter(s):
//   _font:      the standard-14 base font.
//   _size:      point size.
//   _text:      the string to measure.
//   _max_width: the available width in points.
// Return:
//   The count of bytes that fit (0 .. _text.size()).
inline std::size_t
fit_char_count(
    pdf_base_font      _font,
    pdf_unit           _size,
    const std::string& _text,
    pdf_unit           _max_width
) D_NOEXCEPT
{
    const short* table = internal::width_table_for(_font);
    pdf_unit     acc   = 0.0;

    // accumulate until the next glyph would overflow the budget
    for (std::size_t i = 0; i < _text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(_text[i]);
        int           w = (table)
                              ? static_cast<int>(table[c])
                              : internal::courier_width(c);
        pdf_unit      adv = static_cast<pdf_unit>(w) * _size / 1000.0;

        if ((acc + adv) > _max_width)
        {
            return i;
        }

        acc += adv;
    }

    return _text.size();
}

// truncate_ellipsis
//   function: returns _text unchanged if it fits within _max_width,
// otherwise the longest prefix that fits with an ellipsis ("...")
// appended.  If even the ellipsis does not fit, returns "".
//
// Parameter(s):
//   _font:      the standard-14 base font.
//   _size:      point size.
//   _text:      the string to fit.
//   _max_width: the available width in points.
// Return:
//   The fitted (possibly ellipsized) string.
inline std::string
truncate_ellipsis(
    pdf_base_font      _font,
    pdf_unit           _size,
    const std::string& _text,
    pdf_unit           _max_width
)
{
    // whole string already fits - nothing to do
    if (text_width(_font, _size, _text) <= _max_width)
    {
        return _text;
    }

    std::string  ellipsis = "...";
    pdf_unit     ell_w    = text_width(_font, _size, ellipsis);

    // not even room for the ellipsis itself
    if (ell_w > _max_width)
    {
        return std::string();
    }

    std::size_t fit = fit_char_count(
        _font, _size, _text, (_max_width - ell_w));

    return (_text.substr(0, fit) + ellipsis);
}


///////////////////////////////////////////////////////////////////////////////
///                V.   WORD WRAPPING                                        ///
///////////////////////////////////////////////////////////////////////////////

// wrap_to_width
//   function: greedy word-wrap of _text to lines no wider than
// _max_width at the given font and size.  Words are split on ASCII
// spaces; a single word longer than the line is hard-broken at the
// widest character boundary that fits.  Existing newlines in _text
// force line breaks.  Returns the wrapped lines (a blank input
// yields one empty line).
//
// Parameter(s):
//   _font:      the standard-14 base font.
//   _size:      point size.
//   _text:      the string to wrap.
//   _max_width: the available line width in points.
// Return:
//   The wrapped lines, in order.
inline std::vector<std::string>
wrap_to_width(
    pdf_base_font      _font,
    pdf_unit           _size,
    const std::string& _text,
    pdf_unit           _max_width
)
{
    std::vector<std::string> lines;

    std::size_t pos   = 0;
    std::string line;

    // outer loop walks the text one whitespace-delimited token at a
    // time, honoring embedded newlines as forced breaks
    while (pos <= _text.size())
    {
        // find the next break: a space, a newline, or end of string
        std::size_t sp = _text.find(' ',  pos);
        std::size_t nl = _text.find('\n', pos);
        std::size_t br = sp;

        if ( (nl != std::string::npos) &&
             ((br == std::string::npos) || (nl < br)) )
        {
            br = nl;
        }

        std::size_t end =
            (br == std::string::npos) ? _text.size() : br;
        std::string word = _text.substr(pos, end - pos);

        // a word that cannot fit on a line by itself is hard-broken
        if (text_width(_font, _size, word) > _max_width)
        {
            // flush whatever is queued before hard-breaking
            if (!line.empty())
            {
                lines.push_back(line);
                line.clear();
            }

            std::string rest = word;

            // peel off prefixes that fit until the word is consumed
            while (!rest.empty())
            {
                std::size_t fit = fit_char_count(
                    _font, _size, rest, _max_width);

                if (fit == 0)
                {
                    // a single glyph wider than the line: emit it
                    // alone so progress is guaranteed
                    fit = 1;
                }

                lines.push_back(rest.substr(0, fit));
                rest.erase(0, fit);
            }
        }
        else
        {
            // candidate line is the queued text plus this word
            std::string candidate =
                line.empty() ? word : (line + " " + word);

            if (text_width(_font, _size, candidate) > _max_width)
            {
                // does not fit - flush the line and start anew
                lines.push_back(line);
                line = word;
            }
            else
            {
                line = candidate;
            }
        }

        // a newline break flushes the current line immediately
        if ( (br == nl) &&
             (nl != std::string::npos) )
        {
            lines.push_back(line);
            line.clear();
        }

        // advance past the break character (or finish)
        if (end == _text.size())
        {
            break;
        }

        pos = end + 1;
    }

    // emit the trailing line; guarantee at least one line out
    if (!line.empty())
    {
        lines.push_back(line);
    }

    if (lines.empty())
    {
        lines.push_back(std::string());
    }

    return lines;
}

// wrap_to_width
//   function: pdf_font overload using the font's own size.
inline std::vector<std::string>
wrap_to_width(
    const pdf_font&    _font,
    const std::string& _text,
    pdf_unit           _max_width
)
{
    return wrap_to_width(_font.family, _font.size, _text, _max_width);
}


NS_END  // djinterp


#endif  // DJINTERP_TEXT_PDF_METRICS_
