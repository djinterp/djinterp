/******************************************************************************
* djinterp [utility]                                               pdf_metrics.h
*
* Glyph and string width measurement for the standard-14 PDF fonts.
*   This is the keystone of any real layout: centre and right alignment,
* justification, word wrapping and fitting text into a box all reduce to the
* true rendered width of a string, which for proportional faces demands the
* actual Adobe Font Metrics rather than an average-advance approximation.
*
*   THE WIDTH MODEL. The rendered width of a byte at point size S is
* advance_per_1000em[code] * S / 1000, and a string's width is the sum of its
* bytes'. Inputs are single-byte (Latin-1 / WinAnsi) text, matching the
* /Encoding the foundation emits; multi-byte encodings are out of scope.
*   Advances are integer and exact; only the point-size scaling is real-valued.
* The sum is taken in integer and scaled ONCE, where the C++ side scales each
* glyph and sums reals -- so the two can differ in the last bits on long
* strings. Doing it the accurate way here and recording the difference is
* better than reproducing a rounding artefact for a bit-identical answer.
*
*   THE TABLES WERE EXTRACTED MECHANICALLY, NOT RETYPED. 2,560 tabulated
* advances across ten faces is a transcription surface with no natural
* detector: one wrong digit produces widths correct for every string not
* containing that glyph. They were pulled from pdf_metrics.hpp by a script
* asserting 256 values per face, and the differential compares all 3,584 cells
* -- fourteen faces, ten tabulated and four computed -- rather than sampling.
*
*   COURIER IS COMPUTED, NOT TABULATED, but NOT unconditional: the control
* range below 0x20 has no printable glyph and contributes 0, exactly as every
* tabulated face does. The first port returned 600 there, on the reading that
* Courier is "uniformly 600/1000 em" -- true of every printable code and false
* of thirty-two others. The differential caught it as 128 divergent cells.
*   An UNRECOGNISED face falls to the same path, so a garbage enum measures as
* Courier rather than trapping. That is deliberate on both sides -- a metrics
* call is on the layout hot path and has no error channel.
*
*   THE FACE ENUM IS NOT DECLARED HERE. It belongs to pdf_primitives.h, which
* this header includes. It was declared in both once; two enums describing one
* set of faces agree until somebody adds a face to one, and the drift is silent
* because both compile.
*
*
* path:      \inc\djinterp\c\util\pdf\pdf_metrics.h
* link(s):   ch-pdf.tex
* author(s): TBA                                            created: 2026.08.09
******************************************************************************/

#ifndef DJINTERP_C_UTIL_PDF_METRICS_
#define DJINTERP_C_UTIL_PDF_METRICS_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "./pdf_primitives.h"


D_EXTERN_C_BEGIN

// D_PDF_COURIER_ADVANCE
//   constant: the advance of every printable Courier glyph, per 1000 em. Named
// rather than written as a literal in three places, because the three must
// agree and a literal gives no way to say so.
#define D_PDF_COURIER_ADVANCE  600

// d_pdf_wrap_line
//   type: one produced line, as a BORROWED span into the input. Spans rather
// than copies, because wrapping a paragraph would otherwise allocate one string
// per line in a tier that promises no allocation. The spans point into the
// caller's text and are invalidated by anything that moves it.
struct d_pdf_wrap_line
{
    const char* begin;
    size_t      length;
};

// I.   tables
const short* d_pdf_width_table_for(int32_t _font);

// II.  glyph advance
int32_t d_pdf_glyph_advance_em(int32_t _font,
                               int32_t _code);
double  d_pdf_glyph_width(int32_t _font,
                          int32_t _code,
                          double  _size);

// III. string width
double d_pdf_text_width(int32_t     _font,
                        const char* _text,
                        size_t      _length,
                        double      _size);
double d_pdf_text_width_z(int32_t     _font,
                          const char* _text,
                          double      _size);

// IV.  fitting and truncation
size_t d_pdf_fit_char_count(int32_t     _font,
                            const char* _text,
                            size_t      _length,
                            double      _size,
                            double      _max_width);
size_t d_pdf_truncate_ellipsis(int32_t     _font,
                               const char* _text,
                               size_t      _length,
                               double      _size,
                               double      _max_width,
                               char*       _out,
                               size_t      _out_capacity);

// V.   word wrapping
size_t d_pdf_wrap_to_width(int32_t                 _font,
                           const char*             _text,
                           size_t                  _length,
                           double                  _size,
                           double                  _max_width,
                           struct d_pdf_wrap_line* _out,
                           size_t                  _out_capacity);

D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_PDF_METRICS_
