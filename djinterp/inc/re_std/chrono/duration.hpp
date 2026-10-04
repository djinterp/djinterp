/******************************************************************************
* djinterp [re_std]                                                 duration.hpp
*
* the duration class template:
*   A count of ticks together with the compile-time length of one tick:
*
*       duration<long long, milli>   ticks are 1/1000 second
*       duration<double, ratio<60> > ticks are 60 seconds, counted in double
*
*   The period is a re_std::ratio and lives entirely in the type system,
* so no duration carries a scale factor at run time. A duration is exactly
* as large as its representation -- the abstraction is free.
*
*   THE IMPLICIT CONVERSION RULE IS THE HEART OF THE TYPE:
*   A duration converts implicitly to another only when the conversion is
* EXACT. seconds to milliseconds multiplies by 1000 and is implicit;
* milliseconds to seconds divides and would truncate, so it is refused
* and duration_cast must be written by hand. The test is
* ratio_divide<Period2, period>::den == 1 -- the conversion factor being
* a whole number is precisely the condition for losing nothing.
*
*   Floating-point representations opt out of the rule via
* treat_as_floating_point, because every conversion they make is already
* approximate.
*
*   The value of this is that it is a COMPILE-TIME property. A truncation
* cannot happen implicitly in an argument list, in a return statement, or
* anywhere else the reader is not looking.
*
*   CONSTEXPR REACHES FURTHER THAN std's:
*   std made duration's mutating members constexpr in C++17 (P0505R0).
* re_std makes them constexpr from C++14, the first tier whose relaxed
* rules permit mutation in a constant expression -- a three-year lead.
* The const observers are constexpr from C++11, matching std.
*
*   The mutators use D_CONSTEXPR_CPP14 rather than D_CONSTEXPR for a
* second reason beyond the tier: on C++11, constexpr on a member function
* implies const, which would make every one of these a const member
* returning a reference to a modified object -- ill-formed at best and a
* silent duplicate-signature collision at worst.
*
*   C++11 FLOOR: no back-port win here, since std::chrono is itself
* C++11. What re_std adds at this tier is the constexpr reach above and
* one implementation with one behaviour across compilers.
*
*
* path:      /inc/djinterp/re_std/chrono/duration.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_
#define DJINTERP_RE_STD_CHRONO_DURATION_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration_fwd.hpp"
#include "./duration_cast_impl.hpp"
#include "./duration_values.hpp"
#include "./treat_as_floating_point.hpp"
#include "../ratio/ratio.hpp"
#include "../ratio/ratio_divide.hpp"
#include "../type_traits/common_type.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_convertible.hpp"
#include "../cstdint/cstdint.hpp"


NS_RESTD

namespace chrono
{

    // duration
    //   class: a tick count of type _Rep, where one tick is _Period
    // seconds. _Period is a re_std::ratio and is compile-time only.
    template<typename _Rep,
             typename _Period>
    class duration
    {
    public:
        // rep
        //   typedef: the arithmetic type holding the tick count.
        typedef _Rep                            rep;

        // period
        //   typedef: seconds per tick, reduced. ratio normalises at
        // definition, so duration<int, ratio<2,4> >::period is ratio<1,2>
        // and the two spellings name the same specialisation.
        typedef typename _Period::type          period;

    private:
        rep     m_rep;

        // A duration whose rep is itself a duration would nest without
        // limit, and a negative or zero period would invert or collapse
        // the arithmetic. Both are rejected at definition rather than
        // producing a confusing failure deep inside a conversion.
        static_assert(!internal::is_duration<_Rep>::value,
            "re_std::chrono::duration: representation may not be a duration");

        static_assert(period::num > 0,
            "re_std::chrono::duration: period must be positive");

    public:

    // -----------------------------------------------------------------------
    // construction
    // -----------------------------------------------------------------------

        // duration
        //   function: default constructor. The tick count is left
        // UNINITIALISED, exactly as std specifies -- `duration<int> d;`
        // holds garbage, and `duration<int> d{};` holds zero. Defaulting
        // rather than zero-initialising is what keeps duration trivially
        // default constructible and therefore free to place in an
        // uninitialised buffer.
        duration() = default;

        // duration
        //   function: copy constructor. Defaulted, and named explicitly
        // because the converting constructor below is a template that
        // would otherwise be a candidate for copying too.
        duration(const duration&) = default;

        // duration
        //   function: construct from a tick count. Explicit -- a bare
        // integer is not a duration, and allowing the implicit conversion
        // would let a raw 5 pass where a timeout was expected without
        // stating its units.
        //
        //   Constrained on the rep being convertible, and on not
        // narrowing a floating-point count into an integral duration:
        // duration<int>(2.5) would silently become 2, so it is refused.
        template<typename _Rep2,
                 typename = typename enable_if<
                     is_convertible<const _Rep2&, rep>::value &&
                     ( treat_as_floating_point<rep>::value ||
                       !treat_as_floating_point<_Rep2>::value ) >::type>
        D_CONSTEXPR explicit duration(const _Rep2& _r)
            : m_rep(static_cast<rep>(_r))
        {}

        // duration
        //   function: converting constructor. Participates only when the
        // conversion is exact, or when the target rep is floating point.
        // See the header comment -- this constraint is the type's whole
        // reason for existing.
        template<typename _Rep2,
                 typename _Period2,
                 typename = typename enable_if<
                     treat_as_floating_point<rep>::value ||
                     ( ratio_divide<_Period2, period>::den == 1 &&
                       !treat_as_floating_point<_Rep2>::value ) >::type>
        D_CONSTEXPR duration(const duration<_Rep2, _Period2>& _d)
            : m_rep(internal::duration_cast_helper<
                        duration,
                        typename ratio_divide<_Period2, period>::type,
                        typename common_type<rep, _Rep2, std::intmax_t>::type,
                        ratio_divide<_Period2, period>::num == 1,
                        ratio_divide<_Period2, period>::den == 1
                    >::cast(_d).count())
        {}

        // operator=
        //   function: copy assignment. Defaulted.
        duration& operator=(const duration&) = default;

        ~duration() = default;

    // -----------------------------------------------------------------------
    // observer
    // -----------------------------------------------------------------------

        // count
        //   function: the raw tick count, with no scaling applied. The
        // period is not part of the value, so this number is meaningless
        // without the type.
        D_CONSTEXPR rep count() const
        {
            return m_rep;
        }

    // -----------------------------------------------------------------------
    // arithmetic
    // -----------------------------------------------------------------------

        // operator+ (unary)
        //   function: the duration unchanged.
        D_CONSTEXPR duration operator+() const
        {
            return *this;
        }

        // operator- (unary)
        //   function: the negated duration.
        D_CONSTEXPR duration operator-() const
        {
            return duration(-m_rep);
        }

        // operator++ / operator--
        //   function: step the tick count by one tick -- which is one
        // PERIOD, not one second. ++ on a duration<int, milli> advances
        // by a millisecond.
        D_CONSTEXPR_CPP14 duration& operator++()
        {
            ++m_rep;
            return *this;
        }

        D_CONSTEXPR_CPP14 duration operator++(int)
        {
            return duration(m_rep++);
        }

        D_CONSTEXPR_CPP14 duration& operator--()
        {
            --m_rep;
            return *this;
        }

        D_CONSTEXPR_CPP14 duration operator--(int)
        {
            return duration(m_rep--);
        }

        // operator+= / operator-=
        //   function: add or subtract a duration of the SAME type. Mixed
        // periods go through the free operators, which compute a common
        // type -- there is no common type to assign back into here.
        D_CONSTEXPR_CPP14 duration& operator+=(const duration& _d)
        {
            m_rep += _d.count();
            return *this;
        }

        D_CONSTEXPR_CPP14 duration& operator-=(const duration& _d)
        {
            m_rep -= _d.count();
            return *this;
        }

        // operator*= / operator/= / operator%=
        //   function: scale by a scalar of the representation type.
        D_CONSTEXPR_CPP14 duration& operator*=(const rep& _r)
        {
            m_rep *= _r;
            return *this;
        }

        D_CONSTEXPR_CPP14 duration& operator/=(const rep& _r)
        {
            m_rep /= _r;
            return *this;
        }

        D_CONSTEXPR_CPP14 duration& operator%=(const rep& _r)
        {
            m_rep %= _r;
            return *this;
        }

        // operator%=
        //   function: remainder against another duration of the same
        // type.
        D_CONSTEXPR_CPP14 duration& operator%=(const duration& _d)
        {
            m_rep %= _d.count();
            return *this;
        }

    // -----------------------------------------------------------------------
    // special values
    // -----------------------------------------------------------------------

        // zero
        //   function: a duration of no length.
        static D_CONSTEXPR duration zero() D_NOEXCEPT
        {
            return duration(duration_values<rep>::zero());
        }

        // min
        //   function: the most negative representable duration -- not the
        // shortest positive one. See duration_values.hpp.
        static D_CONSTEXPR duration min() D_NOEXCEPT
        {
            return duration(duration_values<rep>::min());
        }

        // max
        //   function: the longest representable duration.
        static D_CONSTEXPR duration max() D_NOEXCEPT
        {
            return duration(duration_values<rep>::max());
        }
    };

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_
