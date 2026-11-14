/*******************************************************************************
* djinterp [cpp/math]                                           t_value_face.cpp
*
*   Tests for math/value.hpp at every level from C++98, against the C core
* (c/math/value.h), which t_value.c checks against an exact reference:
*   - every constructor gives the C core's value, field for field: the
*     integer types at their ends, the unsigned ones past dj_math_imax (G2),
*     the floating-point types, bool, the error codes, dj_rational, ratio,
*     none, an inline value, and a C value copied or adopted;
*   - the queries and every to() overload, written or left as they were;
*   - every operation, its assignment form, unary -, + and abs, on every
*     pair of 57 operands, inline and boxed, for value and for inline_value:
*     the C core's result, field for field, or a boxed result of an equal
*     value under the same table, and each operand's box held as before;
*     the six comparisons, compare, and from C++20 <=>, as value.h's order
*     says;
*   - mixed operands: an int, a double or an inline value meeting a value,
*     and which type each result is;
*   - ownership, with wide integers (c/math/value_wide.h) under a counted
*     copy of their table: a copy retains once, and a value destroyed or
*     overwritten releases once; a move (C++11), swap and adopt neither;
*     detach hands the hold over; a std::vector of values holds each box
*     once per element, and from C++11 moves rather than copies when it
*     grows; storage refused gives a memory error and leaves nothing; and
*     every box made is given back by the end;
*   - from C++11, that a move cannot fail and no pointer converts; from
*     C++14, inline_value's operations as constant expressions; from C++20,
*     value's, <=>, and the concept number.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_value_face.cpp -o t_value_face -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_value_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/value.hpp"  // tested
// std
#include <cfloat>   // FLT_MAX
#include <climits>  // INT_MAX, INT_MIN, LONG_MAX, LONG_MIN, UINT_MAX,
                    // ULONG_MAX
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::sprintf
#include <cstdlib>  // std::malloc, std::free
#include <cstring>  // std::strlen, std::strcmp
#include <limits>   // std::numeric_limits
#include <vector>   // std::vector
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_nothrow_move_constructible, ...
    #include <utility>      // std::move
#endif

// the wide integer's boxes come from the harness, which counts them and
// refuses one where asked: its two knobs, set before value_wide.h is first
// included
void* d_tests_allocate(std::size_t _size);
void  d_tests_give_back(void* _box);
#define D_CFG_MATH_VALUE_WIDE_MALLOC(_size) d_tests_allocate(_size)
#define D_CFG_MATH_VALUE_WIDE_FREE(_ptr)    d_tests_give_back(_ptr)
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/value_wide.h"  // a real boxed kind


namespace
{
    namespace dm = djinterp::math;

    long g_checks   = 0;
    long g_failures = 0;

    // the boxes: held, made, and refused
    long g_live        = 0;
    long g_made        = 0;
    long g_refused     = 0;
    bool g_refuse_next = false;

    // the calls into the counted copy of the wide integer's table
    long g_retains  = 0;
    long g_releases = 0;

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    typedef long long          t_llong;
    typedef unsigned long long t_ullong;
    D_LONG_LONG_DIAG_POP
#endif
}  // namespace

// d_tests_allocate
//   a box's storage, counted; none where the next is to be refused.
void*
d_tests_allocate(
    std::size_t _size
)
{
    // refused, once
    if (g_refuse_next)
    {
        g_refuse_next = false;
        ++g_refused;

        return D_NULLPTR;
    }

    void* box = std::malloc(_size);

    // counted where given
    if (box != D_NULLPTR)
    {
        ++g_live;
        ++g_made;
    }

    return box;
}

// d_tests_give_back
//   a box given back, counted.
void
d_tests_give_back(
    void* _box
)
{
    // counted where there was one
    if (_box != D_NULLPTR)
    {
        --g_live;
    }

    std::free(_box);

    return;
}


namespace
{
    //==========================================================================
    // 1.  REPORTING
    //==========================================================================

    // d_tests_text
    //   _x as text, into _buffer (at least 120 characters), returned.
    const char*
    d_tests_text(
        const dj_math_value& _x,
        char*                _buffer
    )
    {
        char digits[D_MATH_WIDE_TEXT_SIZE];

        // by kind
        if (_x.kind == D_MATH_VALUE_BOXED)
        {
            (void)_x.type->text(digits, sizeof(digits), _x);
            std::sprintf(_buffer, "%s %.60s", _x.type->name, digits);
        }
        else if ( (_x.kind == D_MATH_VALUE_INTEGER) ||
                  (_x.kind == D_MATH_VALUE_RATIONAL) )
        {
            std::sprintf(_buffer, "%s %ld/%ld", d_math_value_kind_name(_x),
                         static_cast<long>(_x.exact.num),
                         static_cast<long>(_x.exact.den));
        }
        else if ( (_x.kind >= D_MATH_VALUE_FLOAT) &&
                  (_x.kind <= D_MATH_VALUE_LONG_DOUBLE) )
        {
            std::sprintf(_buffer, "%s %.21Lg", d_math_value_kind_name(_x),
                         _x.real);
        }
        else
        {
            std::sprintf(_buffer, "%s %d", d_math_value_kind_name(_x),
                         _x.code);
        }

        // the flags, where there are any
        if (_x.flags != 0u)
        {
            std::sprintf(_buffer + std::strlen(_buffer), " (flags %u)",
                         _x.flags);
        }

        return _buffer;
    }

    // d_tests_expect
    //   counts a check, and reports the first few failures with two values
    // that show what was checked, and where.
    void
    d_tests_expect(
        bool                 _ok,
        const char*          _what,
        const dj_math_value& _a,
        const dj_math_value& _b,
        int                  _line
    )
    {
        char text[2][120];

        ++g_checks;

        // report the first few failures
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s (%s; %s)\n", _line, _what,
                            d_tests_text(_a, text[0]),
                            d_tests_text(_b, text[1]));
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what, a, b)                                    \
        d_tests_expect((ok), (what), (a), (b), __LINE__)

    // d_tests_none
    //   none, for a report with nothing to show.
    dj_math_value
    d_tests_none()
    {
        return d_math_value_none();
    }


    //==========================================================================
    // 2.  WHAT A RESULT MUST BE
    //==========================================================================

    // d_tests_same_real
    //   two long doubles the same: equal, a zero's sign included, or both
    // NaN.
    bool
    d_tests_same_real(
        long double _a,
        long double _b
    )
    {
        // NaN
        if (_a != _a)
        {
            return (_b != _b);
        }

        // a zero, by its sign
        if ( (_a == 0.0L) &&
             (_b == 0.0L) )
        {
            return ((1.0L / _a) == (1.0L / _b));
        }

        return (_a == _b);
    }

    // d_tests_same
    //   two C values the same: an inline one field for field; a boxed one,
    // each in a box of its own, under the same table, with the same flags
    // and an equal value.
    bool
    d_tests_same(
        const dj_math_value& _a,
        const dj_math_value& _b
    )
    {
        // boxed: the table, the flags, the value
        if ( (_a.kind == D_MATH_VALUE_BOXED) ||
             (_b.kind == D_MATH_VALUE_BOXED) )
        {
            return ( (_a.kind == _b.kind) &&
                     (_a.type == _b.type) &&
                     (_a.flags == _b.flags) &&
                     (_a.box != D_NULLPTR) &&
                     (_b.box != D_NULLPTR) &&
                     (d_math_value_cmp(_a, _b) == D_MATH_VALUE_EQUAL) );
        }

        return ( (_a.kind == _b.kind) &&
                 (_a.flags == _b.flags) &&
                 (_a.code == _b.code) &&
                 (_a.exact.num == _b.exact.num) &&
                 (_a.exact.den == _b.exact.den) &&
                 (d_tests_same_real(_a.real, _b.real)) &&
                 (_a.type == _b.type) &&
                 (_a.box == _b.box) );
    }

    // d_tests_identical
    //   two C values the very same: inline, field for field; boxed, the same
    // box under the same table.
    bool
    d_tests_identical(
        const dj_math_value& _a,
        const dj_math_value& _b
    )
    {
        return ( (d_tests_same(_a, _b)) &&
                 (_a.box == _b.box) );
    }

    // d_tests_holders
    //   how many values hold the box of a wide integer's value.
    std::size_t
    d_tests_holders(
        const dj_math_value& _x
    )
    {
        return static_cast<const dj_internal_math_value_wide_box*>(
            _x.box)->holders;
    }


    //==========================================================================
    // 3.  THE OPERANDS
    //==========================================================================

    const int MAX_OPERANDS = 64;

    dj_math_value      g_operands[MAX_OPERANDS];
    int                g_operand_count = 0;
    dj_math_boxed_kind g_counted;

    // d_tests_counted_retain, d_tests_counted_release
    //   the wide integer's retain and release, each call counted.
    void
    d_tests_counted_retain(
        void* _box
    )
    {
        ++g_retains;
        d_math_value_wide_kind()->retain(_box);

        return;
    }

    void
    d_tests_counted_release(
        void* _box
    )
    {
        ++g_releases;
        d_math_value_wide_kind()->release(_box);

        return;
    }

    // d_tests_wide
    //   the wide integer of a decimal text.
    dj_math_wide
    d_tests_wide(
        const char* _digits
    )
    {
        dj_math_wide x = d_math_wide_from_imax(0);

        (void)d_math_wide_from_text(&x, _digits, std::strlen(_digits));

        return x;
    }

    // d_tests_wide_value
    //   a wide integer as a C value, the caller's to give back.
    dj_math_value
    d_tests_wide_value(
        const char* _digits
    )
    {
        return d_math_value_from_wide(d_tests_wide(_digits));
    }

    // d_tests_counted_value
    //   the same under the counted copy of the table.
    dj_math_value
    d_tests_counted_value(
        const char* _digits
    )
    {
        dj_math_value made = d_tests_wide_value(_digits);

        made.type = &g_counted;

        return made;
    }

    // d_tests_flagged
    //   _x flagged inexact.
    dj_math_value
    d_tests_flagged(
        dj_math_value _x
    )
    {
        _x.flags = _x.flags | D_MATH_VALUE_FLAG_INEXACT;

        return _x;
    }

    // d_tests_add
    //   _x to the operands.
    void
    d_tests_add(
        const dj_math_value& _x
    )
    {
        // while there is room
        if (g_operand_count < MAX_OPERANDS)
        {
            g_operands[g_operand_count] = _x;
            ++g_operand_count;
        }

        return;
    }

    // d_tests_make_operands
    //   45 inline operands: none, the booleans, the errors; integers and
    // rationals about the family's ends; floats, doubles and long doubles,
    // zeros, infinities and NaNs among them; and two flagged values. Then
    // 12 wide integers, 3 of them under the counted table.
    void
    d_tests_make_operands()
    {
        const dj_math_imax most  = D_MATH_IMAX_MAX;
        const dj_math_imax least = D_MATH_IMAX_MIN;
        const int          half  =
            static_cast<int>(sizeof(dj_math_imax) * CHAR_BIT / 2u);
        const float        f_inf = std::numeric_limits<float>::infinity();
        const float        f_nan = std::numeric_limits<float>::quiet_NaN();
        const double       d_inf = std::numeric_limits<double>::infinity();
        const double       d_nan = std::numeric_limits<double>::quiet_NaN();
        const long double  l_inf =
            std::numeric_limits<long double>::infinity();
        const long double  l_nan =
            std::numeric_limits<long double>::quiet_NaN();

        int code = 0;

        // none, the booleans, the errors
        d_tests_add(d_math_value_none());
        d_tests_add(d_math_value_from_bool(false));
        d_tests_add(d_math_value_from_bool(true));

        for (code = 1; code <= 5; ++code)
        {
            d_tests_add(d_math_value_from_error(
                static_cast<dj_math_value_error>(code)));
        }

        // integers
        d_tests_add(d_math_value_from_int(0));
        d_tests_add(d_math_value_from_int(1));
        d_tests_add(d_math_value_from_int(-1));
        d_tests_add(d_math_value_from_int(2));
        d_tests_add(d_math_value_from_int(-3));
        d_tests_add(d_math_value_from_int(7));
        d_tests_add(d_math_value_from_int(most));
        d_tests_add(d_math_value_from_int(least));
        d_tests_add(d_math_value_from_int(most - 1));
        d_tests_add(
            d_math_value_from_int(static_cast<dj_math_imax>(1) << half));

        // rationals
        d_tests_add(d_math_value_from_ratio(1, 2));
        d_tests_add(d_math_value_from_ratio(-1, 3));
        d_tests_add(d_math_value_from_ratio(2, 3));
        d_tests_add(d_math_value_from_ratio(1, most));
        d_tests_add(d_math_value_from_ratio(most, 2));
        d_tests_add(d_math_value_from_ratio(least, 3));

        // floats, doubles, long doubles
        d_tests_add(d_math_value_from_f(0.0f));
        d_tests_add(d_math_value_from_f(-0.0f));
        d_tests_add(d_math_value_from_f(1.5f));
        d_tests_add(d_math_value_from_f(-2.25f));
        d_tests_add(d_math_value_from_f(FLT_MAX));
        d_tests_add(d_math_value_from_f(f_inf));
        d_tests_add(d_math_value_from_f(f_nan));
        d_tests_add(d_math_value_from_d(0.0));
        d_tests_add(d_math_value_from_d(0.1));
        d_tests_add(d_math_value_from_d(-2.5));
        d_tests_add(d_math_value_from_d(1e300));
        d_tests_add(d_math_value_from_d(9007199254740992.0));
        d_tests_add(d_math_value_from_d(-d_inf));
        d_tests_add(d_math_value_from_d(d_nan));
        d_tests_add(d_math_value_from_ld(0.1L));
        d_tests_add(d_math_value_from_ld(-1e-300L));
        d_tests_add(d_math_value_from_ld(18446744073709551616.0L));
        d_tests_add(d_math_value_from_ld(l_inf));
        d_tests_add(d_math_value_from_ld(l_nan));

        // flagged: G2's long double, and an integer
        d_tests_add(d_math_value_add(d_math_value_from_int(most),
                                     d_math_value_from_int(1)));
        d_tests_add(d_tests_flagged(d_math_value_from_int(5)));

        // wide integers, under the kind's own table
        d_tests_add(d_tests_wide_value("0"));
        d_tests_add(d_tests_wide_value("5"));
        d_tests_add(d_tests_wide_value("-7"));
        d_tests_add(d_tests_wide_value("18446744073709551617"));
        d_tests_add(d_tests_wide_value("-1267650600228229401496703205376"));
        d_tests_add(d_tests_wide_value("1000000000000000000000000000000000"
                                       "0000000000000000000000000000000000"
                                       "000000000"));
        d_tests_add(d_tests_wide_value("578960446186580977117854925043439"
                                       "539266349923328202820197287920039"
                                       "56564819949"));
        d_tests_add(d_tests_wide_value("-11579208923731619542357098500868"
                                       "790785326998466564056403945758400"
                                       "7913129639935"));
        d_tests_add(d_tests_flagged(d_tests_wide_value("12")));

        // and under the counted copy
        d_tests_add(d_tests_counted_value("5"));
        d_tests_add(d_tests_counted_value("-1267650600228229401496703205376"));
        d_tests_add(d_tests_counted_value("578960446186580977117854925043439"
                                          "539266349923328202820197287920039"
                                          "56564819949"));

        return;
    }

    // d_tests_release_operands
    //   every operand given back.
    void
    d_tests_release_operands()
    {
        int i = 0;

        for (i = 0; i < g_operand_count; ++i)
        {
            d_math_value_release(&g_operands[i]);
        }

        g_operand_count = 0;

        return;
    }

    // d_tests_inline_of
    //   an inline C value as an inline_value, through value's to().
    dm::inline_value
    d_tests_inline_of(
        const dj_math_value& _x
    )
    {
        dm::inline_value result;

        (void)dm::value(_x).to(result);

        return result;
    }


    //==========================================================================
    // 4.  CONSTRUCTION
    //==========================================================================

    // d_tests_constructed
    //   a value and an inline value both _want, field for field, and alike.
    void
    d_tests_constructed(
        const dm::value&        _made,
        const dm::inline_value& _made_inline,
        const dj_math_value&    _want,
        const char*             _what
    )
    {
        D_TESTS_EXPECT( (d_tests_same(_made.c_value(), _want)) &&
                        (d_tests_same(_made_inline.c_value(), _want)),
                        _what, _made.c_value(), _want);

        return;
    }

    // d_tests_construction
    //   every constructor, against the C core's.
    void
    d_tests_construction()
    {
        const dj_math_imax most = D_MATH_IMAX_MAX;

        // none
        d_tests_constructed(dm::value(), dm::inline_value(),
                            d_math_value_none(), "none");

        // the signed integer types, at their ends
        d_tests_constructed(dm::value(0), dm::inline_value(0),
                            d_math_value_from_int(0), "int 0");
        d_tests_constructed(dm::value(INT_MAX), dm::inline_value(INT_MAX),
                            d_math_value_from_int(INT_MAX), "int's greatest");
        d_tests_constructed(dm::value(INT_MIN), dm::inline_value(INT_MIN),
                            d_math_value_from_int(INT_MIN), "int's least");
        d_tests_constructed(dm::value(LONG_MAX), dm::inline_value(LONG_MAX),
                            d_math_value_from_int(LONG_MAX), "long's greatest");
        d_tests_constructed(dm::value(LONG_MIN), dm::inline_value(LONG_MIN),
                            d_math_value_from_int(LONG_MIN), "long's least");
        d_tests_constructed(dm::value(most), dm::inline_value(most),
                            d_math_value_from_int(most),
                            "dj_math_imax's greatest");

        // promoted: char, short, unsigned char
        d_tests_constructed(dm::value('A'), dm::inline_value('A'),
                            d_math_value_from_int('A'), "a char");
        d_tests_constructed(dm::value(static_cast<short>(-5)),
                            dm::inline_value(static_cast<short>(-5)),
                            d_math_value_from_int(-5), "a short");
        d_tests_constructed(dm::value(static_cast<unsigned char>(200)),
                            dm::inline_value(static_cast<unsigned char>(200)),
                            d_math_value_from_int(200), "an unsigned char");

        // the unsigned integer types: an integer where it fits, past it G2
        d_tests_constructed(dm::value(0u), dm::inline_value(0u),
                            d_math_value_from_uint(0u), "unsigned 0");
        d_tests_constructed(dm::value(UINT_MAX), dm::inline_value(UINT_MAX),
                            d_math_value_from_uint(UINT_MAX),
                            "unsigned's greatest");
        d_tests_constructed(dm::value(ULONG_MAX), dm::inline_value(ULONG_MAX),
                            d_math_value_from_uint(ULONG_MAX),
                            "unsigned long's greatest");
        d_tests_constructed(dm::value(static_cast<std::size_t>(12345u)),
                            dm::inline_value(static_cast<std::size_t>(12345u)),
                            d_math_value_from_uint(12345u), "a size_t");

#if D_ENV_HAS_LONG_LONG
        {
            const t_ullong u_all = static_cast<t_ullong>(0) - 1u;
            const t_llong  s_most = static_cast<t_llong>(u_all >> 1);
            const t_llong  s_least = -s_most - 1;

            d_tests_constructed(dm::value(s_most), dm::inline_value(s_most),
                                d_math_value_from_int(s_most),
                                "long long's greatest");
            d_tests_constructed(dm::value(s_least), dm::inline_value(s_least),
                                d_math_value_from_int(s_least),
                                "long long's least");
            d_tests_constructed(dm::value(u_all), dm::inline_value(u_all),
                                d_math_value_from_uint(u_all),
                                "unsigned long long's greatest");
        }
#endif

        // the floating-point types
        d_tests_constructed(dm::value(1.5f), dm::inline_value(1.5f),
                            d_math_value_from_f(1.5f), "a float");
        d_tests_constructed(dm::value(-0.0), dm::inline_value(-0.0),
                            d_math_value_from_d(-0.0), "a double, -0");
        d_tests_constructed(dm::value(0.1L), dm::inline_value(0.1L),
                            d_math_value_from_ld(0.1L), "a long double");
        d_tests_constructed(
            dm::value(std::numeric_limits<double>::quiet_NaN()),
            dm::inline_value(std::numeric_limits<double>::quiet_NaN()),
            d_math_value_from_d(std::numeric_limits<double>::quiet_NaN()),
            "a double NaN");

        // bool, the error codes, dj_rational
        d_tests_constructed(dm::value(true), dm::inline_value(true),
                            d_math_value_from_bool(true), "true");
        d_tests_constructed(dm::value(false), dm::inline_value(false),
                            d_math_value_from_bool(false), "false");
        d_tests_constructed(dm::value(D_MATH_VALUE_ERROR_POLE),
                            dm::inline_value(D_MATH_VALUE_ERROR_POLE),
                            d_math_value_from_error(D_MATH_VALUE_ERROR_POLE),
                            "an error code");
        {
            const dj_rational three_quarters = { 3, 4 };
            const dj_rational six_over_one   = { 6, 1 };

            d_tests_constructed(dm::value(three_quarters),
                                dm::inline_value(three_quarters),
                                d_math_value_from_rational(three_quarters),
                                "a dj_rational");
            d_tests_constructed(dm::value(six_over_one),
                                dm::inline_value(six_over_one),
                                d_math_value_from_rational(six_over_one),
                                "a whole dj_rational");
        }

        // ratio, reduced, whole, and over 0
        d_tests_constructed(dm::value::ratio(2, -4),
                            dm::inline_value::ratio(2, -4),
                            d_math_value_from_ratio(2, -4), "ratio 2/-4");
        d_tests_constructed(dm::value::ratio(6, 3),
                            dm::inline_value::ratio(6, 3),
                            d_math_value_from_ratio(6, 3), "ratio 6/3");
        d_tests_constructed(dm::value::ratio(1, 0),
                            dm::inline_value::ratio(1, 0),
                            d_math_value_from_ratio(1, 0), "ratio 1/0");
        d_tests_constructed(dm::value::ratio(0, 0),
                            dm::inline_value::ratio(0, 0),
                            d_math_value_from_ratio(0, 0), "ratio 0/0");
        d_tests_constructed(dm::value::ratio(D_MATH_IMAX_MIN, -1),
                            dm::inline_value::ratio(D_MATH_IMAX_MIN, -1),
                            d_math_value_from_ratio(D_MATH_IMAX_MIN, -1),
                            "ratio least/-1, past the family");

        // an inline value as a value, and a value's inline value
        {
            const dm::inline_value third = dm::inline_value::ratio(1, 3);
            const dm::value        held  = third;
            dm::inline_value       back;

            D_TESTS_EXPECT( (d_tests_same(held.c_value(), third.c_value())) &&
                            (held.to(back)) &&
                            (d_tests_same(back.c_value(), third.c_value())),
                            "an inline value as a value, and back",
                            held.c_value(), third.c_value());
        }

        // a C value: copied, or adopted
        {
            const dj_math_value c_value = d_math_value_from_ratio(5, 7);
            const dm::value     copied(c_value);
            const dm::value     adopted = dm::value::adopt(c_value);

            D_TESTS_EXPECT( (d_tests_same(copied.c_value(), c_value)) &&
                            (d_tests_same(adopted.c_value(), c_value)),
                            "a C value, copied and adopted",
                            copied.c_value(), adopted.c_value());
        }

        return;
    }


    //==========================================================================
    // 5.  ONE OPERAND
    //==========================================================================

    // d_tests_queries
    //   _made's queries, against value.h's on _x.
    template<typename Made>
    bool
    d_tests_queries(
        const Made&          _made,
        const dj_math_value& _x
    )
    {
        return ( (_made.kind() == _x.kind) &&
                 (_made.flags() == _x.flags) &&
                 (_made.rank() == d_math_value_rank(_x)) &&
                 (std::strcmp(_made.kind_name(),
                              d_math_value_kind_name(_x)) == 0) &&
                 (_made.is_number() == d_math_value_is_number(_x)) &&
                 (_made.is_exact() == d_math_value_is_exact(_x)) );
    }

    // d_tests_conversions
    //   _made's to() overloads, against value.h's on _x: the same answer,
    // and the same value written, or the sentinel left.
    template<typename Made>
    bool
    d_tests_conversions(
        const Made&          _made,
        const dj_math_value& _x
    )
    {
        const float       f_sentinel  = -123.0f;
        const double      d_sentinel  = -123.0;
        const long double ld_sentinel = -123.0L;
        const dj_rational r_sentinel  = { -123, 7 };

        float               f       = f_sentinel;
        double              d       = d_sentinel;
        long double         ld      = ld_sentinel;
        dj_rational         r       = r_sentinel;
        dj_math_imax        n       = -123;
        bool                truth   = false;
        dj_math_value_error error   = D_MATH_VALUE_ERROR_RANGE;
        float               want_f  = f_sentinel;
        double              want_d  = d_sentinel;
        long double         want_ld = ld_sentinel;
        dj_rational         want_r  = r_sentinel;

        const bool to_f  = _made.to(f);
        const bool to_d  = _made.to(d);
        const bool to_ld = _made.to(ld);
        const bool to_r  = _made.to(r);
        const bool to_n  = _made.to(n);
        const bool to_b  = _made.to(truth);
        const bool to_e  = _made.to(error);

        const bool c_f  = d_math_value_to_f(&want_f, _x);
        const bool c_d  = d_math_value_to_d(&want_d, _x);
        const bool c_ld = d_math_value_to_ld(&want_ld, _x);
        const bool c_r  = d_math_value_to_rational(&want_r, _x);
        const bool c_n  = ( (c_r) &&
                            (want_r.den == 1) );
        const bool c_b  = (_x.kind == D_MATH_VALUE_BOOLEAN);
        const bool c_e  = (_x.kind == D_MATH_VALUE_ERROR);

        return ( (to_f == c_f) &&
                 (d_tests_same_real(f, want_f)) &&
                 (to_d == c_d) &&
                 (d_tests_same_real(d, want_d)) &&
                 (to_ld == c_ld) &&
                 (d_tests_same_real(ld, want_ld)) &&
                 (to_r == c_r) &&
                 (r.num == want_r.num) &&
                 (r.den == want_r.den) &&
                 (to_n == c_n) &&
                 (n == (c_n ? want_r.num : -123)) &&
                 (to_b == c_b) &&
                 (truth == (c_b ? (_x.code != 0) : false)) &&
                 (to_e == c_e) &&
                 (error == (c_e ? static_cast<dj_math_value_error>(_x.code)
                                : D_MATH_VALUE_ERROR_RANGE)) );
    }

    // d_tests_one
    //   _x alone: as a value (and an inline value, where inline), its
    // queries and conversions; -x, +x and |x| as value.h's; and the box
    // held as before.
    void
    d_tests_one(
        const dj_math_value& _x
    )
    {
        const bool        boxed   = (_x.kind == D_MATH_VALUE_BOXED);
        const std::size_t holders = (boxed) ? d_tests_holders(_x) : 0u;
        const long        live    = g_live;

        {
            const dm::value  x(_x);
            dm::inline_value as_inline;
            dj_math_value    neg = d_math_value_neg(_x);
            dj_math_value    abs_x = d_math_value_abs(_x);

            // a copy of a C value: the same box, held once more
            D_TESTS_EXPECT( (d_tests_identical(x.c_value(), _x)) &&
                            ( (!boxed) ||
                              (d_tests_holders(_x) == holders + 1u) ),
                            "a copy of a C value", x.c_value(), _x);

            D_TESTS_EXPECT(d_tests_queries(x, _x), "the queries", _x,
                           d_tests_none());
            D_TESTS_EXPECT(d_tests_conversions(x, _x), "the conversions", _x,
                           d_tests_none());
            D_TESTS_EXPECT( (x.to(as_inline) == !boxed) &&
                            ( (boxed) ? (as_inline.kind() ==
                                         D_MATH_VALUE_NONE)
                                      : (d_tests_same(as_inline.c_value(),
                                                      _x)) ),
                            "to an inline value", _x, as_inline.c_value());

            D_TESTS_EXPECT(d_tests_same((-x).c_value(), neg), "-x",
                           (-x).c_value(), neg);
            D_TESTS_EXPECT(d_tests_identical((+x).c_value(), _x), "+x",
                           (+x).c_value(), _x);
            D_TESTS_EXPECT(d_tests_same(abs(x).c_value(), abs_x), "|x|",
                           abs(x).c_value(), abs_x);

            // inline: the same as an inline value
            if (!boxed)
            {
                const dm::inline_value y = d_tests_inline_of(_x);

                D_TESTS_EXPECT( (d_tests_queries(y, _x)) &&
                                (d_tests_conversions(y, _x)) &&
                                (d_tests_same((-y).c_value(), neg)) &&
                                (d_tests_same((+y).c_value(), _x)) &&
                                (d_tests_same(abs(y).c_value(), abs_x)),
                                "an inline value alone", _x, y.c_value());
            }

            d_math_value_release(&neg);
            d_math_value_release(&abs_x);
        }

        // nothing left held
        D_TESTS_EXPECT( (g_live == live) &&
                        ( (!boxed) ||
                          (d_tests_holders(_x) == holders) ),
                        "nothing left held, alone", _x, d_tests_none());

        return;
    }


    //==========================================================================
    // 6.  TWO OPERANDS
    //==========================================================================

    // d_tests_c_op
    //   _a _op _b by value.h: 0 +, 1 -, 2 *, 3 /.
    dj_math_value
    d_tests_c_op(
        int                  _op,
        const dj_math_value& _a,
        const dj_math_value& _b
    )
    {
        // by operation
        switch (_op)
        {
            case 0:
                return d_math_value_add(_a, _b);

            case 1:
                return d_math_value_sub(_a, _b);

            case 2:
                return d_math_value_mul(_a, _b);

            default:
                return d_math_value_div(_a, _b);
        }
    }

    // d_tests_op, d_tests_op_assign
    //   _a _op _b by the face, as an operator, and as its assignment form.
    template<typename Value>
    Value
    d_tests_op(
        int          _op,
        const Value& _a,
        const Value& _b
    )
    {
        // by operation
        switch (_op)
        {
            case 0:
                return _a + _b;

            case 1:
                return _a - _b;

            case 2:
                return _a * _b;

            default:
                return _a / _b;
        }
    }

    template<typename Value>
    Value
    d_tests_op_assign(
        int          _op,
        const Value& _a,
        const Value& _b
    )
    {
        Value result(_a);

        // by operation
        switch (_op)
        {
            case 0:
                result += _b;
                break;

            case 1:
                result -= _b;
                break;

            case 2:
                result *= _b;
                break;

            default:
                result /= _b;
                break;
        }

        return result;
    }

    // d_tests_comparisons
    //   _a against _b by the face, as value.h orders them.
    template<typename Value>
    bool
    d_tests_comparisons(
        const Value&        _a,
        const Value&        _b,
        dj_math_value_order _order
    )
    {
        bool ok = ( (_a.compare(_b) == _order) &&
                    ((_a == _b) == (_order == D_MATH_VALUE_EQUAL)) &&
                    ((_a != _b) == (_order != D_MATH_VALUE_EQUAL)) &&
                    ((_a < _b) == (_order == D_MATH_VALUE_LESS)) &&
                    ((_a > _b) == (_order == D_MATH_VALUE_GREATER)) &&
                    ((_a <= _b) == ( (_order == D_MATH_VALUE_LESS) ||
                                     (_order == D_MATH_VALUE_EQUAL) )) &&
                    ((_a >= _b) == ( (_order == D_MATH_VALUE_GREATER) ||
                                     (_order == D_MATH_VALUE_EQUAL) )) );

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        {
            const std::partial_ordering want =
                (_order == D_MATH_VALUE_LESS)
                    ? std::partial_ordering::less
                    : (_order == D_MATH_VALUE_EQUAL)
                          ? std::partial_ordering::equivalent
                          : (_order == D_MATH_VALUE_GREATER)
                                ? std::partial_ordering::greater
                                : std::partial_ordering::unordered;

            ok = ( (ok) &&
                   ((_a <=> _b) == want) );
        }
#endif

        return ok;
    }

    // d_tests_two
    //   _a and _b: each operation, as an operator and its assignment form,
    // against value.h's, for value and, both inline, inline_value; the
    // comparisons; and the operands' boxes held as before, nothing left.
    void
    d_tests_two(
        const dj_math_value& _a,
        const dj_math_value& _b
    )
    {
        const bool        a_boxed   = (_a.kind == D_MATH_VALUE_BOXED);
        const bool        b_boxed   = (_b.kind == D_MATH_VALUE_BOXED);
        const std::size_t a_holders = (a_boxed) ? d_tests_holders(_a) : 0u;
        const std::size_t b_holders = (b_boxed) ? d_tests_holders(_b) : 0u;
        const long        live      = g_live;

        int op = 0;

        {
            const dm::value a(_a);
            const dm::value b(_b);

            for (op = 0; op < 4; ++op)
            {
                dj_math_value want = d_tests_c_op(op, _a, _b);

                {
                    const dm::value made        = d_tests_op(op, a, b);
                    const dm::value made_assign = d_tests_op_assign(op, a, b);

                    D_TESTS_EXPECT(d_tests_same(made.c_value(), want),
                                   "an operator", _a, _b);
                    D_TESTS_EXPECT(d_tests_same(made_assign.c_value(), want),
                                   "an assignment form", _a, _b);
                }

                // both inline: the same as inline values
                if ( (!a_boxed) &&
                     (!b_boxed) )
                {
                    const dm::inline_value ia = d_tests_inline_of(_a);
                    const dm::inline_value ib = d_tests_inline_of(_b);

                    D_TESTS_EXPECT( (d_tests_same(d_tests_op(op, ia,
                                                             ib).c_value(),
                                                  want)) &&
                                    (d_tests_same(d_tests_op_assign(op, ia,
                                                                    ib)
                                                      .c_value(),
                                                  want)),
                                    "an inline value's operator", _a, _b);
                }

                d_math_value_release(&want);
            }

            D_TESTS_EXPECT(d_tests_comparisons(a, b, d_math_value_cmp(_a, _b)),
                           "the comparisons", _a, _b);

            // both inline: the same as inline values
            if ( (!a_boxed) &&
                 (!b_boxed) )
            {
                D_TESTS_EXPECT(d_tests_comparisons(d_tests_inline_of(_a),
                                                   d_tests_inline_of(_b),
                                                   d_math_value_cmp(_a, _b)),
                               "an inline value's comparisons", _a, _b);
            }
        }

        // the operands' boxes held as before, and nothing left
        D_TESTS_EXPECT( (g_live == live) &&
                        ( (!a_boxed) ||
                          (d_tests_holders(_a) == a_holders) ) &&
                        ( (!b_boxed) ||
                          (d_tests_holders(_b) == b_holders) ),
                        "nothing left held, two", _a, _b);

        return;
    }

    // d_tests_pairs
    //   every operand alone, and every ordered pair.
    void
    d_tests_pairs()
    {
        int i = 0;
        int j = 0;

        for (i = 0; i < g_operand_count; ++i)
        {
            d_tests_one(g_operands[i]);
        }

        for (i = 0; i < g_operand_count; ++i)
        {
            for (j = 0; j < g_operand_count; ++j)
            {
                d_tests_two(g_operands[i], g_operands[j]);
            }
        }

        return;
    }


    //==========================================================================
    // 7.  MIXED OPERANDS
    //==========================================================================

    // d_tests_type
    //   which face type a result is: 1 a value, 2 an inline value.
    int
    d_tests_type(
        const dm::value&
    )
    {
        return 1;
    }

    int
    d_tests_type(
        const dm::inline_value&
    )
    {
        return 2;
    }

    // d_tests_mixed
    //   an int, a double or an inline value meeting a value or an inline
    // value: converted to it, which type each result is, and value.h's
    // result.
    void
    d_tests_mixed()
    {
        const dm::value        v     = dm::value::ratio(1, 2);
        const dm::inline_value iv    = 3;
        const dj_math_value    half  = d_math_value_from_ratio(1, 2);
        const dj_math_value    three = d_math_value_from_int(3);

        // the types
        D_TESTS_EXPECT( (d_tests_type(iv + 1) == 2) &&
                        (d_tests_type(1 - iv) == 2) &&
                        (d_tests_type(iv * 2.5) == 2) &&
                        (d_tests_type(-iv) == 2) &&
                        (d_tests_type(abs(iv)) == 2) &&
                        (d_tests_type(iv + v) == 1) &&
                        (d_tests_type(v / iv) == 1) &&
                        (d_tests_type(v - 1) == 1) &&
                        (d_tests_type(2.0 * v) == 1) &&
                        (d_tests_type(abs(v)) == 1),
                        "an inline value with a number stays inline; with a "
                        "value it is a value", v.c_value(), iv.c_value());

        // the results
        {
            dj_math_value sum   = d_math_value_add(half, three);
            dj_math_value diff  = d_math_value_sub(d_math_value_from_int(1),
                                                   half);
            dj_math_value prod  = d_math_value_mul(d_math_value_from_d(2.0),
                                                   half);
            dj_math_value quot  = d_math_value_div(three,
                                                   d_math_value_from_int(4));
            dj_math_value large = d_math_value_add(
                d_math_value_from_uint(D_MATH_UMAX_MAX), half);

            D_TESTS_EXPECT( (d_tests_same((v + iv).c_value(), sum)) &&
                            (d_tests_same((iv + v).c_value(), sum)) &&
                            (d_tests_same((1 - v).c_value(), diff)) &&
                            (d_tests_same((2.0 * v).c_value(), prod)) &&
                            (d_tests_same((iv / 4).c_value(), quot)) &&
                            (d_tests_same((D_MATH_UMAX_MAX + v).c_value(),
                                          large)),
                            "mixed results", sum, large);

            d_math_value_release(&sum);
            d_math_value_release(&diff);
            d_math_value_release(&prod);
            d_math_value_release(&quot);
            d_math_value_release(&large);
        }

        // the comparisons
        D_TESTS_EXPECT( (v < 1) &&
                        (1 > v) &&
                        (v == 0.5) &&
                        (0.5 == v) &&
                        (v != iv) &&
                        (iv > v) &&
                        (iv == 3) &&
                        (3 <= iv) &&
                        (!(v == true)) &&
                        (v != true) &&
                        (!(dm::value(true) < dm::value(1))) &&
                        (dm::value(false) < dm::value(true)) &&
                        (dm::value() == dm::value()) &&
                        (!(dm::value(D_MATH_VALUE_ERROR_POLE) ==
                           dm::value(D_MATH_VALUE_ERROR_POLE))),
                        "mixed comparisons", v.c_value(), iv.c_value());

        return;
    }


    //==========================================================================
    // 8.  OWNERSHIP
    //==========================================================================

    // d_tests_calls
    //   whether the counted table was called _retains and _releases times
    // since _since_retains and _since_releases.
    bool
    d_tests_calls(
        long _since_retains,
        long _since_releases,
        long _retains,
        long _releases
    )
    {
        return ( (g_retains - _since_retains == _retains) &&
                 (g_releases - _since_releases == _releases) );
    }

    // d_tests_copies
    //   a copy retains once; a value destroyed releases once; adopt neither;
    // a C value copied retains once; detach hands the hold over.
    void
    d_tests_copies()
    {
        long retains  = g_retains;
        long releases = g_releases;

        const dm::value five =
            dm::value::adopt(d_tests_counted_value("5"));

        // adopted: no holder more
        D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 0)) &&
                        (d_tests_holders(five.c_value()) == 1u),
                        "adopt holds the C value's box as it is",
                        five.c_value(), d_tests_none());

        // a copy: one retain, the same box
        {
            const dm::value copy(five);

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 0)) &&
                            (d_tests_holders(five.c_value()) == 2u) &&
                            (d_tests_identical(copy.c_value(),
                                               five.c_value())),
                            "a copy retains once", copy.c_value(),
                            five.c_value());
        }

        D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 1)) &&
                        (d_tests_holders(five.c_value()) == 1u),
                        "a value destroyed releases once", five.c_value(),
                        d_tests_none());

        // a C value copied: one retain; detached: the hold handed over
        retains  = g_retains;
        releases = g_releases;
        {
            dm::value     held(five.c_value());
            dj_math_value detached = d_math_value_none();

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 0)) &&
                            (d_tests_holders(five.c_value()) == 2u),
                            "a C value copied retains once", held.c_value(),
                            five.c_value());

            detached = held.detach();

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 0)) &&
                            (held.kind() == D_MATH_VALUE_NONE) &&
                            (d_tests_identical(detached, five.c_value())) &&
                            (d_tests_holders(five.c_value()) == 2u),
                            "detach hands the hold over, and leaves none",
                            detached, held.c_value());

            d_math_value_release(&detached);
        }

        D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 1)) &&
                        (d_tests_holders(five.c_value()) == 1u),
                        "a detached hold, given back by C, and none "
                        "destroyed", five.c_value(), d_tests_none());

        return;
    }

    // d_tests_assignments
    //   copy assignment: the new value retained, the old released, and a
    // value assigned itself held as before.
    void
    d_tests_assignments()
    {
        const dm::value five =
            dm::value::adopt(d_tests_counted_value("5"));
        const dm::value seven =
            dm::value::adopt(d_tests_counted_value("-7"));

        long retains  = g_retains;
        long releases = g_releases;

        {
            dm::value  target = 3;
            dm::value& alias  = target;

            // a box over an inline value: one retain
            target = five;

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 1, 0)) &&
                            (d_tests_identical(target.c_value(),
                                               five.c_value())) &&
                            (d_tests_holders(five.c_value()) == 2u),
                            "a box assigned over an inline value",
                            target.c_value(), five.c_value());

            // a box over a box: one retain, one release
            target = seven;

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 2, 1)) &&
                            (d_tests_identical(target.c_value(),
                                               seven.c_value())) &&
                            (d_tests_holders(five.c_value()) == 1u) &&
                            (d_tests_holders(seven.c_value()) == 2u),
                            "a box assigned over a box", target.c_value(),
                            seven.c_value());

            // itself: held as before
            target = alias;

            D_TESTS_EXPECT( (d_tests_identical(target.c_value(),
                                               seven.c_value())) &&
                            (d_tests_holders(seven.c_value()) == 2u) &&
                            ( (g_retains - retains) -
                              (g_releases - releases) == 1 ),
                            "a value assigned itself", target.c_value(),
                            seven.c_value());

            // an inline value over a box: one release
            retains  = g_retains;
            releases = g_releases;
            target   = 4;

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 1)) &&
                            (target == 4) &&
                            (d_tests_holders(seven.c_value()) == 1u),
                            "an inline value assigned over a box",
                            target.c_value(), seven.c_value());

            // an operation's result over a box: the old one released
            target  = seven;
            retains = g_retains;
            releases = g_releases;
            target += five;

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 1)) &&
                            (target == -2) &&
                            (target.kind() == D_MATH_VALUE_BOXED) &&
                            (d_tests_holders(target.c_value()) == 1u) &&
                            (d_tests_holders(seven.c_value()) == 1u) &&
                            (d_tests_holders(five.c_value()) == 1u),
                            "an assignment form over a box",
                            target.c_value(), seven.c_value());

            // itself, as an operand
            target = seven;
            target *= alias;

            D_TESTS_EXPECT( (target == 49) &&
                            (d_tests_holders(seven.c_value()) == 1u),
                            "a value times itself, in place",
                            target.c_value(), seven.c_value());
        }

        // the only holder, assigned itself: the box held as before
        {
            dm::value  only  = dm::value::adopt(d_tests_counted_value("9"));
            dm::value& alias = only;
            const long live  = g_live;

            only = alias;

            D_TESTS_EXPECT( (g_live == live) &&
                            (d_tests_holders(only.c_value()) == 1u) &&
                            (only == 9),
                            "the only holder assigned itself", only.c_value(),
                            d_tests_none());
        }

        D_TESTS_EXPECT( (d_tests_holders(five.c_value()) == 1u) &&
                        (d_tests_holders(seven.c_value()) == 1u),
                        "every assignment's hold given back", five.c_value(),
                        seven.c_value());

        return;
    }

    // d_tests_moves_and_swaps
    //   from C++11 a move neither retains nor releases, and leaves none;
    // swap does neither at any level.
    void
    d_tests_moves_and_swaps()
    {
        const dm::value five =
            dm::value::adopt(d_tests_counted_value("5"));

        long retains  = g_retains;
        long releases = g_releases;

        // swap, the member and the free function
        {
            dm::value a(five);
            dm::value b = dm::value::adopt(d_tests_counted_value("-7"));

            retains  = g_retains;
            releases = g_releases;

            a.swap(b);

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 0)) &&
                            (a == -7) &&
                            (d_tests_identical(b.c_value(), five.c_value())),
                            "swap, the member", a.c_value(), b.c_value());

            swap(a, b);

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 0)) &&
                            (b == -7) &&
                            (d_tests_identical(a.c_value(), five.c_value())),
                            "swap, the free function", a.c_value(),
                            b.c_value());
        }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        {
            dm::value source(five);

            retains  = g_retains;
            releases = g_releases;

            dm::value moved(std::move(source));

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 0)) &&
                            (source.kind() == D_MATH_VALUE_NONE) &&
                            (d_tests_identical(moved.c_value(),
                                               five.c_value())) &&
                            (d_tests_holders(five.c_value()) == 2u),
                            "a move, constructing", moved.c_value(),
                            source.c_value());

            dm::value  target =
                dm::value::adopt(d_tests_counted_value("-7"));
            dm::value& alias = target;
            const long live  = g_live;

            retains  = g_retains;
            releases = g_releases;
            target   = std::move(moved);

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 1)) &&
                            (moved.kind() == D_MATH_VALUE_NONE) &&
                            (d_tests_identical(target.c_value(),
                                               five.c_value())) &&
                            (d_tests_holders(five.c_value()) == 2u) &&
                            (g_live == live - 1),
                            "a move, assigning", target.c_value(),
                            moved.c_value());

            // itself: held as before
            retains  = g_retains;
            releases = g_releases;
            target   = std::move(alias);

            D_TESTS_EXPECT( (d_tests_calls(retains, releases, 0, 0)) &&
                            (d_tests_identical(target.c_value(),
                                               five.c_value())) &&
                            (d_tests_holders(five.c_value()) == 2u),
                            "a value moved into itself", target.c_value(),
                            five.c_value());
        }
#endif

        D_TESTS_EXPECT(d_tests_holders(five.c_value()) == 1u,
                       "every move's and swap's hold given back",
                       five.c_value(), d_tests_none());

        return;
    }

    // d_tests_containers
    //   a std::vector of values holds each box once per element, gives each
    // back as it goes, and from C++11 moves rather than copies as it grows.
    void
    d_tests_containers()
    {
        const dm::value five =
            dm::value::adopt(d_tests_counted_value("5"));
        const long      retains  = g_retains;
        const long      releases = g_releases;

        {
            std::vector<dm::value> many;
            int                    i = 0;

            for (i = 0; i < 100; ++i)
            {
                many.push_back(five);
            }

            D_TESTS_EXPECT( (d_tests_holders(five.c_value()) == 101u) &&
                            ( (g_retains - retains) -
                              (g_releases - releases) == 100 ),
                            "a vector holds each box once per element",
                            five.c_value(), d_tests_none());

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
            D_TESTS_EXPECT( (g_retains - retains == 100) &&
                            (g_releases == releases),
                            "a vector moves its values as it grows",
                            five.c_value(), d_tests_none());
#endif

            many.erase(many.begin(), many.begin() + 50);

            D_TESTS_EXPECT(d_tests_holders(five.c_value()) == 51u,
                           "a vector's erased values given back",
                           five.c_value(), d_tests_none());
        }

        D_TESTS_EXPECT(d_tests_holders(five.c_value()) == 1u,
                       "a vector's values given back", five.c_value(),
                       d_tests_none());

        return;
    }

    // d_tests_operations_hold
    //   an operation leaves its operands' holders as they were, and its
    // result is a box of its own, given back when it is destroyed.
    void
    d_tests_operations_hold()
    {
        const dm::value five =
            dm::value::adopt(d_tests_counted_value("5"));
        const long      live = g_live;

        {
            const dm::value sum = five + five;
            const dm::value neg = -five;

            D_TESTS_EXPECT( (d_tests_holders(five.c_value()) == 1u) &&
                            (g_live == live + 2) &&
                            (sum.c_value().box != five.c_value().box) &&
                            (d_tests_holders(sum.c_value()) == 1u) &&
                            (sum.c_value().type == &g_counted) &&
                            (sum == 10) &&
                            (neg == -5),
                            "an operation's result, a box of its own",
                            sum.c_value(), neg.c_value());
        }

        D_TESTS_EXPECT( (g_live == live) &&
                        (d_tests_holders(five.c_value()) == 1u),
                        "an operation's result given back", five.c_value(),
                        d_tests_none());

        return;
    }

    // d_tests_refused
    //   where the kind has no storage for a result: a memory error, as
    // value.h's own, and nothing left; a copy needs none.
    void
    d_tests_refused()
    {
        const dm::value big =
            dm::value::adopt(d_tests_wide_value("18446744073709551617"));
        const long      live = g_live;

        {
            dj_math_value want = d_math_value_none();

            g_refuse_next = true;
            want          = d_math_value_add(big.c_value(),
                                             d_math_value_from_int(1));
            g_refuse_next = true;

            const dm::value sum = big + 1;

            g_refuse_next = true;

            const dm::value neg = -big;

            g_refuse_next = true;

            const dm::value copy(big);

            g_refuse_next = false;

            D_TESTS_EXPECT( (want.kind == D_MATH_VALUE_ERROR) &&
                            (want.code ==
                             static_cast<int>(D_MATH_VALUE_ERROR_MEMORY)) &&
                            (d_tests_same(sum.c_value(), want)) &&
                            (d_tests_same(neg.c_value(), want)) &&
                            (d_tests_identical(copy.c_value(),
                                               big.c_value())),
                            "storage refused: a memory error", sum.c_value(),
                            want);
        }

        D_TESTS_EXPECT( (g_live == live) &&
                        (d_tests_holders(big.c_value()) == 1u),
                        "storage refused: nothing left", big.c_value(),
                        d_tests_none());

        return;
    }

    // d_tests_ownership
    //   ownership, each part above.
    void
    d_tests_ownership()
    {
        d_tests_copies();
        d_tests_assignments();
        d_tests_moves_and_swaps();
        d_tests_containers();
        d_tests_operations_hold();
        d_tests_refused();

        return;
    }


    //==========================================================================
    // 9.  CONSTANT EVALUATION
    //==========================================================================

#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )

    // d_tests_ce_sum
    //   1/3 + 1/6.
    constexpr dm::inline_value
    d_tests_ce_sum()
    {
        return dm::inline_value::ratio(1, 3) + dm::inline_value::ratio(1, 6);
    }

    // d_tests_ce_assign
    //   2, times 0.5, less 3, in place: the double -2.
    constexpr dm::inline_value
    d_tests_ce_assign()
    {
        dm::inline_value x = 2;

        x *= dm::inline_value(0.5);
        x -= 3;

        return x;
    }

    // d_tests_ce_past
    //   dj_math_imax's greatest, plus 1: G2's long double.
    constexpr dm::inline_value
    d_tests_ce_past()
    {
        return dm::inline_value(D_MATH_IMAX_MAX) + 1;
    }

    // d_tests_ce_whole
    //   6/3's dj_math_imax.
    constexpr dj_math_imax
    d_tests_ce_whole()
    {
        dj_math_imax whole = 0;

        return (dm::inline_value::ratio(6, 3).to(whole)) ? whole : -1;
    }

    // d_tests_ce_unsigned
    //   dj_math_umax's greatest: G2's long double.
    constexpr dm::inline_value
    d_tests_ce_unsigned()
    {
        return dm::inline_value(D_MATH_UMAX_MAX);
    }

#endif  // C++14

}  // namespace

#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
static_assert(d_tests_ce_sum() == dm::inline_value::ratio(1, 2),
              "1/3 + 1/6 is 1/2, an inline value's constant expression");
static_assert(d_tests_ce_sum().kind() == D_MATH_VALUE_RATIONAL,
              "and a rational");
static_assert( (d_tests_ce_assign().kind() == D_MATH_VALUE_DOUBLE) &&
               (d_tests_ce_assign() == -2),
               "the assignment forms in a constant expression");
static_assert( (d_tests_ce_past().kind() == D_MATH_VALUE_LONG_DOUBLE) &&
               (d_tests_ce_past().flags() == D_MATH_VALUE_FLAG_INEXACT),
               "past dj_math_imax, G2's flagged long double");
static_assert(d_tests_ce_whole() == 2, "to() in a constant expression");
static_assert( (d_tests_ce_unsigned().kind() == D_MATH_VALUE_LONG_DOUBLE) &&
               (d_tests_ce_unsigned().flags() == D_MATH_VALUE_FLAG_INEXACT),
               "an unsigned past dj_math_imax, G2's flagged long double");
static_assert( (dm::inline_value(1) < dm::inline_value(1.5)) &&
               (abs(dm::inline_value(-3)) == 3) &&
               ((-dm::inline_value::ratio(1, 2)).compare(0) ==
                D_MATH_VALUE_LESS) &&
               (dm::inline_value(D_MATH_VALUE_ERROR_POLE).compare(1) ==
                D_MATH_VALUE_UNORDERED) &&
               (dm::inline_value(std::numeric_limits<double>::quiet_NaN()) !=
                dm::inline_value(std::numeric_limits<double>::quiet_NaN())) &&
               (dm::inline_value(true).rank() == 0) &&
               (dm::inline_value(2.5f).rank() == D_MATH_VALUE_RANK_FLOAT),
               "an inline value's comparisons and queries in a constant "
               "expression");
#endif

#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
namespace
{
    // d_tests_ce_value
    //   a value's copies, moves, swap and assignment form, as a constant
    // expression: 3/2.
    constexpr dm::value
    d_tests_ce_value()
    {
        dm::value x = 1;
        dm::value y = x;

        y += dm::value::ratio(1, 2);

        dm::value z = std::move(y);

        swap(x, z);

        return x;
    }

    // d_tests_ce_detach
    //   a value detached: its C value, and none left.
    constexpr bool
    d_tests_ce_detach()
    {
        dm::value           x      = 7;
        const dj_math_value held   = x.detach();
        dm::inline_value    inlined;

        return ( (held.kind == D_MATH_VALUE_INTEGER) &&
                 (held.exact.num == 7) &&
                 (x.kind() == D_MATH_VALUE_NONE) &&
                 (dm::value(held).to(inlined)) &&
                 (inlined == 7) );
    }
}  // namespace

static_assert(d_tests_ce_value() == dm::value::ratio(3, 2),
              "a value's ownership in a constant expression (C++20)");
static_assert(d_tests_ce_detach(), "detach in a constant expression");
static_assert( ((dm::value(1) <=> dm::value(2)) ==
                std::partial_ordering::less) &&
               ((dm::value(2.0) <=> dm::value(2)) ==
                std::partial_ordering::equivalent) &&
               ((dm::value(D_MATH_VALUE_ERROR_POLE) <=> dm::value(1)) ==
                std::partial_ordering::unordered) &&
               ((dm::inline_value::ratio(1, 3) <=> 0.25) ==
                std::partial_ordering::greater),
               "<=> as a constant expression");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert( (dm::number<dm::value>) &&
               (dm::number<dm::inline_value>) &&
               (dm::number<int>) &&
               (dm::number<double>) &&
               (dm::number<long double>) &&
               (!dm::number<bool>) &&
               (!dm::number<const char*>) &&
               (!dm::number<void*>),
               "the concept number");
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
static_assert( (std::is_nothrow_move_constructible<dm::value>::value) &&
               (std::is_nothrow_move_assignable<dm::value>::value) &&
               (std::is_nothrow_copy_constructible<dm::value>::value),
               "a value's moves and copies cannot fail (C++11)");
static_assert( (!std::is_constructible<dm::value, const char*>::value) &&
               (!std::is_constructible<dm::value, int*>::value) &&
               (!std::is_constructible<dm::inline_value, void*>::value),
               "no pointer converts to a value (C++11)");
static_assert( (std::is_trivially_destructible<dm::inline_value>::value) &&
               (!std::is_trivially_destructible<dm::value>::value),
               "an inline value holds nothing to give back (C++11)");
static_assert( (std::is_convertible<int, dm::value>::value) &&
               (std::is_convertible<dm::inline_value, dm::value>::value) &&
               (!std::is_convertible<dm::value, dm::inline_value>::value) &&
               (!std::is_convertible<dj_math_value, dm::value>::value) &&
               (std::is_constructible<dm::value, dj_math_value>::value),
               "the conversions, implicit and explicit (C++11)");
#endif

namespace
{
    // d_tests_constant_evaluation
    //   the constant expressions, at run time.
    void
    d_tests_constant_evaluation()
    {
#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
        D_TESTS_EXPECT( (d_tests_ce_sum() == dm::inline_value::ratio(1, 2)) &&
                        (d_tests_ce_assign() == -2) &&
                        (d_tests_ce_past().flags() ==
                         D_MATH_VALUE_FLAG_INEXACT) &&
                        (d_tests_ce_whole() == 2) &&
                        (d_tests_ce_unsigned().flags() ==
                         D_MATH_VALUE_FLAG_INEXACT),
                        "the constant expressions, at run time",
                        d_tests_ce_sum().c_value(),
                        d_tests_ce_assign().c_value());
#endif
#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
        D_TESTS_EXPECT( (d_tests_ce_value() == dm::value::ratio(3, 2)) &&
                        (d_tests_ce_detach()),
                        "a value's constant expressions, at run time",
                        d_tests_ce_value().c_value(), d_tests_none());
#endif

        return;
    }
}  // namespace


int
main()
{
    g_counted         = *d_math_value_wide_kind();
    g_counted.retain  = d_tests_counted_retain;
    g_counted.release = d_tests_counted_release;

    d_tests_make_operands();
    d_tests_construction();
    d_tests_pairs();
    d_tests_mixed();
    d_tests_ownership();
    d_tests_constant_evaluation();
    d_tests_release_operands();

    // every box given back, and only the refusals asked for refused
    D_TESTS_EXPECT( (g_live == 0) &&
                    (g_made > 0) &&
                    (g_refused == 3),
                    "every box given back", d_tests_none(), d_tests_none());

    std::printf("t_value_face: %ld checks, %ld failures\n", g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
