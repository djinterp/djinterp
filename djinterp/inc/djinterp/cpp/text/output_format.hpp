/*******************************************************************************
* djinterp [cpp]                                               output_format.hpp
*
*   Number formatting onto a DOutputStream: the C++ face of
* c/text/ostream_format.h. A double in the C locale's form, whatever the
* program's locale -- write_double(os, value) and os << value -- a double in a
* locale the caller names for one call, write_double(os, value, locale) over a
* DOutputLocale, and printf-formatted text, write_formatted(os, format, ...),
* which follows the locale as printf does. The names are the C functions' own
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
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NUMBER FORMATTING
    -----------------
    1.  Doubles
    2.  printf
    3.  Locales
         1.  DOutputLocale
         2.  write_double in a locale
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


// 1.3    Locales
//------------------------------------------------------------------------------
// 1.3.1
// DOutputLocale
//   class: a numeric locale a double is written in for one call, over the C
// tier's dj_ostream_locale, which it makes and releases. Not copyable; moved
// from C++11. Read-only once made, so threads may share one.
class DOutputLocale
{
public:
    /**
     * @brief The C form, as write_double(_os, _value) writes.
     */
    DOutputLocale() D_NOEXCEPT
    {
        (void)d_ostream_locale_init(&m_locale, "C");
    }

    /**
     * @brief A named locale; ok() says whether it could be made.
     *
     * @param[in] _name  a locale's name, as setlocale takes it; "" for the
     *                   one the environment names.
     */
    explicit DOutputLocale(
        const char* _name
    ) D_NOEXCEPT
    {
        (void)d_ostream_locale_init(&m_locale, _name);
    }

    /**
     * @brief Releases the platform's locale object.
     */
    ~DOutputLocale()
    {
        d_ostream_locale_close(&m_locale);
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Takes _other's locale. C++11 and later, ADDITIVE.
     *
     * @param[in,out] _other  the locale to take; left no locale.
     */
    DOutputLocale(
        DOutputLocale&& _other
    ) D_NOEXCEPT
        : m_locale(_other.m_locale)
    {
        _other.m_locale.handle = D_NULLPTR;
        _other.m_locale.is_c   = 0;
    }

    /**
     * @brief Releases this locale, then takes _other's. C++11 and later,
     *        ADDITIVE.
     *
     * @param[in,out] _other  the locale to take; left no locale.
     * @return *this.
     */
    DOutputLocale& operator=(
        DOutputLocale&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            d_ostream_locale_close(&m_locale);
            m_locale               = _other.m_locale;
            _other.m_locale.handle = D_NULLPTR;
            _other.m_locale.is_c   = 0;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    /**
     * @brief Whether this is a locale: the C form, or a named locale that
     *        could be made. A write with one that is not is refused.
     *
     * @return true when it is.
     */
    D_NODISCARD bool ok() const D_NOEXCEPT
    {
        return ( (m_locale.is_c != 0) ||
                 (m_locale.handle != D_NULLPTR) );
    }

    /**
     * @brief The C tier's locale, for d_ostream_write_double_l.
     *
     * @return the locale; valid while this object lives.
     */
    D_NODISCARD const struct dj_ostream_locale* get() const D_NOEXCEPT
    {
        return &m_locale;
    }

    /**
     * @brief Whether this build can make a named locale.
     *
     * @return true when it can; otherwise only the C form is available.
     */
    static bool named_supported() D_NOEXCEPT
    {
        return d_ostream_locale_named() != 0;
    }

private:
    D_DELETED_FN(DOutputLocale(const DOutputLocale& _other))
    D_DELETED_FN(DOutputLocale& operator=(const DOutputLocale& _other))

    struct dj_ostream_locale m_locale;
};

// 1.3.2
/**
 * @brief Writes a double in a locale: its decimal point, and nothing else of
 *        it. The program's locale is neither read nor changed.
 *
 * @note A locale that is not ok() is refused, and recorded in the stream as
 *       D_OSTREAM_INVALID: a choice that would change nothing is not taken
 *       silently.
 *
 * @param[in,out] _os         the stream.
 * @param[in]     _value      the value.
 * @param[in]     _locale     the locale.
 * @param[in]     _style      'g', 'f' or 'e', as printf's conversions.
 * @param[in]     _precision  printf's precision; negative for its default, 6.
 * @return true while the stream is healthy, as DOutputStream::write().
 */
inline bool write_double(
    DOutputStream&       _os,
    double               _value,
    const DOutputLocale& _locale,
    char                 _style     = 'g',
    int32_t              _precision = -1
) D_NOEXCEPT
{
    return d_ostream_write_double_l(_os.get(),
                                    _value,
                                    _style,
                                    _precision,
                                    _locale.get()) != 0;
}


NS_END  // djinterp


#endif  // DJINTERP_TEXT_OUTPUT_FORMAT_HPP
