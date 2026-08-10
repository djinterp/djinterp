/******************************************************************************
* djinterp [utility]                                       pdf_builtin_backend.h
*
* Serialization primitives for the built-in PDF writer.
*   Escaping, number formatting, colour operators, date stamps and object
* markers -- the byte-level vocabulary a PDF content stream is written in.
*
*   EVERY EMITTER TAKES A CALLER BUFFER AND RETURNS THE LENGTH IT NEEDED, in
* snprintf's sense: the return is what WOULD have been written, so a caller
* sizes by calling with a null buffer and retries. The C++ originals return
* std::string, which this tier has no allocator for. A caller wanting one call
* rather than two can use the D_PDF_*_MAX bounds below, all of which are exact
* rather than generous.
*
*   THE DECIMAL POINT IS A SHARED HAZARD, NOT A PORTING CHOICE. `pdf_num` on
* the C++ side formats with "%g", and its own comment hedges: "locale-
* independent-ENOUGH". It is not. %g takes its decimal separator from
* LC_NUMERIC, so under a German or French locale a coordinate emits as "1,5"
* -- which is not a number in a PDF content stream, and produces a file that
* every reader rejects. A document that renders correctly on one machine
* silently corrupts on another with no code change.
*   This port emits "%g" ANYWAY, deliberately. Fixing it here alone would mean
* the two tiers write different bytes for the same document under exactly the
* locales where it matters, and the differential -- which runs under the C
* locale -- would never see the difference. It belongs in both tiers as a
* ruling, and until then both are wrong in the same way, which is at least
* detectable. The fixture pins the C-locale output so any unilateral change
* shows up as a divergence rather than as a silent improvement.
*
*   THE DATE IS LOCAL TIME, and therefore not reproducible. `pdf_creation_date`
* stamps a document with the machine's wall clock in its own timezone, so two
* builds of one document differ -- which defeats byte-comparison of output and
* is worth knowing before anyone tries it. The C++ side does the same; a
* reproducible build wants the caller to supply the timestamp instead, which is
* why `d_pdf_format_date` is exposed separately from `d_pdf_creation_date`.
*
*   THE BACKEND CLASS IS NOT HERE. `builtin_pdf_backend` implements nine pure
* virtuals, and its C form is a struct of function pointers plus a context --
* who owns the context, what a null slot means, whether the table is versioned.
* Those are decisions, not translations, and they belong with the module that
* first dispatches through the table. This header carries the primitives that
* class is built out of, which need no dispatch at all.
*
*
* path:      \inc\djinterp\c\util\pdf\pdf_builtin_backend.h
* link(s):   ch-pdf.tex
* author(s): TBA                                            created: 2026.08.09
******************************************************************************/

#ifndef DJINTERP_C_UTIL_PDF_BUILTIN_BACKEND_
#define DJINTERP_C_UTIL_PDF_BUILTIN_BACKEND_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "./pdf_primitives.h"


D_EXTERN_C_BEGIN

// D_PDF_NUM_MAX
//   constant: bytes a formatted scalar can occupy, excluding the terminator.
// "%g" emits at most sign, 17 significant digits, a point and a five-character
// exponent; 31 covers that with room and is a round number a caller can put on
// the stack without thinking about it.
#define D_PDF_NUM_MAX  31

// D_PDF_DATE_MAX
//   constant: bytes in a PDF date string, excluding the terminator. The form
// is "D:YYYYMMDDHHmmSS" -- exactly 16, and fixed, so a caller may size to it
// rather than query.
#define D_PDF_DATE_MAX  16

// D_PDF_ESCAPE_WORST_CASE
//   constant: the factor by which escaping can expand a run. A byte outside
// printable ASCII becomes a four-character octal escape ("\\303"), so the
// bound is four bytes out per byte in -- not three, which is the count of
// octal digits and the number a reader is likely to guess.
#define D_PDF_ESCAPE_WORST_CASE  4

// I.   escaping
size_t d_pdf_escape_text(const char* _text,
                         size_t      _length,
                         char*       _out,
                         size_t      _out_capacity);

// II.  scalars
size_t d_pdf_num(double _value,
                 char*  _out,
                 size_t _out_capacity);

// III. colour operators
size_t d_pdf_set_color(const struct d_pdf_color* _color,
                       int32_t                   _fill,
                       char*                     _out,
                       size_t                    _out_capacity);

// IV.  dates
size_t d_pdf_format_date(int64_t _unix_seconds,
                         int32_t _use_local_time,
                         char*   _out,
                         size_t  _out_capacity);
size_t d_pdf_creation_date(char*  _out,
                           size_t _out_capacity);

// V.   object markers
size_t d_pdf_begin_object(char*   _out,
                          size_t  _out_capacity,
                          size_t  _out_length,
                          size_t* _offsets,
                          size_t  _offset_count,
                          int32_t _object_number);

// VI.  layout assertions
D_STATIC_ASSERT(D_PDF_DATE_MAX == 16,
                "a PDF date is D:YYYYMMDDHHmmSS -- exactly sixteen bytes");
D_STATIC_ASSERT(D_PDF_ESCAPE_WORST_CASE == 4,
                "an octal escape is a backslash and three digits");

D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_PDF_BUILTIN_BACKEND_
