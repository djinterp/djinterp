/*******************************************************************************
* djinterp [cpp]                                               output_format.hpp
*
*   Number formatting onto a DOutputStream: the C++ face of
* c/text/ostream_format.h. A double in the C locale's form, whatever the
* program's locale -- write_double(os, value) and os << value -- and
* printf-formatted text, write_formatted(os, format, ...), which follows the
* locale as printf does. The names are the C functions' own
* (d_ostream_write_double, d_ostream_printf), and stay clear of writer.hpp's
* print(value, printer).
*
*   WHY THESE ARE NOT MEMBERS. The stream's kernel sits below text (ruling 3):
* archive, compress, net, DTest and logging write bytes through it without
* needing text. Integers stay with the kernel, converted in place with no
* locale; a double's formatting and printf are text's. Free functions keep
* DOutputStream's header free of them, and a unit that includes this header
* writes a double exactly as the kernel's members once did. Without it,
* os << 1.5 does not compile -- each integer overload is as good a match as
* another -- so a double is never silently written as an integer.
*
*   write_formatted IS NOT CALLED printf. A function named printf in
* namespace djinterp would hide ::printf from every unqualified call inside
* the namespace.
*
*   C++98 AND UP. A C++98 compiler has no va_copy, so the va_list goes to the
* C side once, as d_ostream_vprintf takes it.
*
*
* path:      /inc/djinterp/cpp/text/output_format.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NUMBER FORMATTING
    -----------------
    1.  Doubles
    2.  printf
*/

#ifndef DJINTERP_TEXT_OUTPUT_FORMAT_HPP
#define DJINTERP_TEXT_OUTPUT_FORMAT_HPP 1

// std
#include <cstdarg>  // va_list, va_start, va_end
// djinterp
#include "../djinterp.hpp"                   // framework root
#include "../../c/text/ostream_format.h"     // d_ostream_write_double,
                                             // d_ostream_vprintf
#include "../util/output/output_stream.hpp"  // DOutputStream
// re_std
#include "../../../re_std/cpp/cstdint/cstdint.hpp"  // int32_t


NS_DJINTERP


//==============================================================================
// 1.  NUMBER FORMATTING
//==============================================================================


// 1.1    Doubles
//------------------------------------------------------------------------------
/**
 * @brief Writes a double in the C locale's form, whatever the locale.
 *
 * @note Not-a-number writes as "nan" and the infinities as "inf" and "-inf".
 *
 * @param[in,out] _os         the stream.
 * @param[in]     _value      the value.
 * @param[in]     _style      'g', 'f' or 'e', as printf's conversions.
 * @param[in]     _precision  printf's precision; negative for its default, 6.
 * @return true while the stream is healthy, as DOutputStream::write().
 */
inline bool write_double(
    DOutputStream& _os,
    double         _value,
    char           _style     = 'g',
    int32_t        _precision = -1
) D_NOEXCEPT
{
    return d_ostream_write_double(_os.get(),
                                  _value,
                                  _style,
                                  _precision) != 0;
}

/**
 * @brief Writes a double as write_double(_os, _value) does, chained.
 *
 * @param[in,out] _os     the stream; check it once after the chain.
 * @param[in]     _value  the value.
 * @return _os.
 */
inline DOutputStream& operator<<(
    DOutputStream& _os,
    double         _value
) D_NOEXCEPT
{
    (void)write_double(_os, _value);

    return _os;
}

// 1.2    printf
//------------------------------------------------------------------------------
/**
 * @brief Writes printf-formatted text; it follows the locale, as printf
 *        does.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _format  the format.
 * @param[in]     ...      the values.
 * @return true while the stream is healthy, as DOutputStream::write().
 */
inline bool write_formatted(
    DOutputStream& _os,
    const char*    _format,
    ...
) D_NOEXCEPT
{
    va_list args;

    va_start(args, _format);

    const int32_t result = d_ostream_vprintf(_os.get(), _format, args);

    va_end(args);

    return result != 0;
}


NS_END  // djinterp


#endif  // DJINTERP_TEXT_OUTPUT_FORMAT_HPP
