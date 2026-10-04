/******************************************************************************
* djinterp [re_std]                                          chrono_literals.hpp
*
* the chrono user-defined literals:
*   `1s`, `250ms`, `2h` and the rest, in re_std::literals::chrono_literals.
*
*   THE TWO OVERLOADS PER SUFFIX ARE NOT REDUNDANT:
*   Each suffix has an integer form and a long-double form. `1s` is
* seconds -- an integral duration -- while `1.5s` is
* duration<double, ratio<1> >, a different type. Without the second
* overload `1.5s` would not compile at all; with it, fractional literals
* work and carry their fractional part into an appropriate
* representation.
*
*   THE INTEGER FORMS RETURN THE PREDEFINED TYPEDEFS EXACTLY:
*   `1s` is seconds, not duration<int>. This matters: it means a literal
* interoperates with the rest of the library without a conversion, and it
* means `auto timeout = 30s;` gives a variable with the same type a
* function signature would use.
*
*   WHY THE NESTED NAMESPACE IS WORTH RESPECTING:
*   The suffixes are short, common tokens -- s, h, min, d, y -- and the
* standard confines them to a nested inline namespace so a program opts
* in deliberately:
*
*       using namespace re_std::literals::chrono_literals;
*       using namespace re_std::literals;          // also works
*       using namespace re_std::chrono;            // also works
*
*   All three reach the suffixes, matching std's arrangement, because
* chrono_literals is inline within literals and literals is reopened
* inside chrono. Do not add a using-directive for any of these at
* namespace scope in a header: `s` is a name other libraries also want.
*
*   `d` AND `y` ARE THE DURATION FORMS, NOT THE CALENDAR ONES:
*   In C++20 these two suffixes are overloaded -- in a calendar context
* `15d` is a chrono::day and `2020y` is a chrono::year. re_std has no
* calendar yet, so here they are unambiguously days and years, the
* duration typedefs. When the calendar lands these literals must be
* revisited; that is recorded in the roadmap rather than worked around.
*
*   C++11 FLOOR: user-defined literals are a C++11 feature. std added the
* chrono suffixes in C++14 and d / y in C++20, so this is a three-year
* back-port for the first six and a nine-year one for the last two.
*
*
* path:      /inc/djinterp/re_std/chrono/chrono_literals.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_CHRONO_LITERALS_
#define DJINTERP_RE_STD_CHRONO_CHRONO_LITERALS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_typedefs.hpp"
#include "../ratio/ratio.hpp"
#include "../ratio/ratio_typedefs.hpp"


NS_RESTD

namespace literals
{
inline namespace chrono_literals
{

    // operator""h
    //   function: hours. `2h` is hours(2); `1.5h` is a floating-point
    // duration of ninety minutes.
    D_CONSTEXPR chrono::hours operator"" h(unsigned long long _v)
    {
        return chrono::hours(static_cast<chrono::hours::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, ratio<3600> >
    operator"" h(long double _v)
    {
        return chrono::duration<long double, ratio<3600> >(_v);
    }

    // operator""min
    //   function: minutes. Spelled `min` rather than `m` because `m`
    // would collide with a metres suffix in any program that has one.
    D_CONSTEXPR chrono::minutes operator"" min(unsigned long long _v)
    {
        return chrono::minutes(static_cast<chrono::minutes::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, ratio<60> >
    operator"" min(long double _v)
    {
        return chrono::duration<long double, ratio<60> >(_v);
    }

    // operator""s
    //   function: seconds. Note this suffix is also used by
    // basic_string's literal in std; the two live in different nested
    // namespaces and are distinguished by the operand type.
    D_CONSTEXPR chrono::seconds operator"" s(unsigned long long _v)
    {
        return chrono::seconds(static_cast<chrono::seconds::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double> operator"" s(long double _v)
    {
        return chrono::duration<long double>(_v);
    }

    // operator""ms
    //   function: milliseconds.
    D_CONSTEXPR chrono::milliseconds operator"" ms(unsigned long long _v)
    {
        return chrono::milliseconds(
            static_cast<chrono::milliseconds::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, milli>
    operator"" ms(long double _v)
    {
        return chrono::duration<long double, milli>(_v);
    }

    // operator""us
    //   function: microseconds.
    D_CONSTEXPR chrono::microseconds operator"" us(unsigned long long _v)
    {
        return chrono::microseconds(
            static_cast<chrono::microseconds::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, micro>
    operator"" us(long double _v)
    {
        return chrono::duration<long double, micro>(_v);
    }

    // operator""ns
    //   function: nanoseconds.
    D_CONSTEXPR chrono::nanoseconds operator"" ns(unsigned long long _v)
    {
        return chrono::nanoseconds(
            static_cast<chrono::nanoseconds::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, nano>
    operator"" ns(long double _v)
    {
        return chrono::duration<long double, nano>(_v);
    }

    // operator""d
    //   function: days, the DURATION. See the header comment -- in C++20
    // this suffix also has a calendar meaning re_std does not yet
    // implement.
    D_CONSTEXPR chrono::days operator"" d(unsigned long long _v)
    {
        return chrono::days(static_cast<chrono::days::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, ratio<86400> >
    operator"" d(long double _v)
    {
        return chrono::duration<long double, ratio<86400> >(_v);
    }

    // operator""y
    //   function: years, the DURATION -- the average Gregorian year, not
    // a calendar year. Same caveat as `d`.
    D_CONSTEXPR chrono::years operator"" y(unsigned long long _v)
    {
        return chrono::years(static_cast<chrono::years::rep>(_v));
    }

    D_CONSTEXPR chrono::duration<long double, ratio<31556952> >
    operator"" y(long double _v)
    {
        return chrono::duration<long double, ratio<31556952> >(_v);
    }

}  // namespace chrono_literals
}  // namespace literals


namespace chrono
{
    // Reopened so that `using namespace re_std::chrono;` also brings the
    // suffixes in, exactly as [time.syn] arranges for std::chrono.
    using namespace literals::chrono_literals;
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_CHRONO_LITERALS_
