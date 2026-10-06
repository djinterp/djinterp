/*******************************************************************************
* djinterp [math]                                                   interval.hpp
*
* Unified compile-time interval template.
*   This header provides a single, fully generic interval type parameterized
* by value type, bounds, boundary openness, optional discrete step, and
* size type. It subsumes the functionality of closed_interval,
* open_interval, and discrete_interval into one template and exposes
* compile-time conversion aliases between all interval configurations.
*
*   A face over the C core (c/math/interval.h, interval_float.h), as
* closed_interval.hpp is: every operation forwards to
* internal::interval_kernel<Type>. An interval's members are the values its
* type holds within its bounds: integers, or for a floating-point bound
* (C++20) every representable value, an open bound moving to the
* neighbouring one. overlaps compares ranges; intersects asks for a shared
* member. Everything compiles from C++98; the operations are constexpr from
* C++14, the alias templates exist from C++11 (with_step and its kin have
* C++98 forms, rebind_step and its kin), and the _v variable templates from
* C++14.
*
* TEMPLATE PARAMETERS:
*   Type      - the value/element type (any arithmetic or ordered type)
*   Lower     - the lower bound
*   Upper     - the upper bound
*   LeftOpen  - true if the left endpoint is excluded  (default: false)
*   RightOpen - true if the right endpoint is excluded (default: false)
*   Step      - discrete stride; 0 means continuous    (default: 0)
*   SizeType  - unsigned type used for counts/indices  (default: size_t)
* BOUNDARY CONFIGURATIONS:
*   [a, b]   closed        LeftOpen=false, RightOpen=false  (default)
*   (a, b)   open          LeftOpen=true,  RightOpen=true
*   [a, b)   half-open-R   LeftOpen=false, RightOpen=true
*   (a, b]   half-open-L   LeftOpen=true,  RightOpen=false
* DISCRETE vs CONTINUOUS:
*   Step == 0  => continuous (unit-stride iteration for integral types)
*   Step >  0  => discrete   (stride-Step iteration, alignment checks)
* CONVERSION TYPES (nested):
*   as_closed, as_open, as_half_open_left, as_half_open_right,
*   as_continuous, rebind_step<S>::type, rebind_bounds<L,U>::type,
*   rebind_size_type<S>::type; from C++11 with_step<S>, with_bounds<L,U>,
*   with_size_type<S>
* FREE-STANDING CONVERSION METAFUNCTIONS:
*   internal::interval_cast_to_closed<Source>::type and its kin; from C++11
*   to_closed_interval_t<Source> and its kin
* STRUCTURAL INTERFACE (for interval_traits):
*   value_type, size_type, lower_bound, upper_bound, step,
*   is_left_open, is_right_open
*
*
* path:      /inc/djinterp/math/interval/interval.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.23
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UNIFIED INTERVAL
    ----------------
    1.  interval
    2.  Static member definitions
2.  INTER-TYPE CONVERSION METAFUNCTIONS
    -----------------------------------
3.  CONVENIENCE TYPE ALIASES (C++11)
    --------------------------------
4.  INTERVAL TRAITS
    ---------------
    1.  Detection helpers
    2.  Interval detection
    3.  Boundary type detection
    4.  Endpoint detection
    5.  Interval property detection
    6.  Interval type extraction
    7.  Interval relationship traits
    8.  Variable templates (C++14)
*/

#ifndef DJINTERP_MATH_INTERVAL_INTERVAL_HPP
#define DJINTERP_MATH_INTERVAL_INTERVAL_HPP 1

// std
#include <cstddef>                         // std::size_t
#include <string>                          // std::string
// djinterp
#include "../../djinterp.hpp"              // framework root
#include "../../core/meta/trait_detect.hpp"  // D_TYPE_TRAIT_HAS_STATIC_MEMBER,
                                             // D_TYPE_TRAIT_HAS_TYPE
#include "./interval_common.hpp"           // internal::interval_kernel,
                                           // interval_iterator
#include "./closed_interval.hpp"           // closed_interval
#include "./open_interval.hpp"             // open_interval
#include "./discrete_interval.hpp"         // discrete_interval
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int8_t ... uint64_t
#include "../../../re_std/type_traits/integral_constant.hpp"  // integral_
                                                              // constant
#include "../../../re_std/type_traits/is_same.hpp"            // is_same


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  UNIFIED INTERVAL
//==============================================================================


// 1.1    interval
//------------------------------------------------------------------------------
// interval
//   struct: compile-time interval over Type with configurable bounds,
// boundary openness, discrete step, and size type. When Step is 0 the
// interval is continuous; when Step > 0 it is discrete.
template<typename Type,
         Type     Lower,
         Type     Upper,
         bool     LeftOpen  = false,
         bool     RightOpen = false,
         Type     Step      = static_cast<Type>(0),
         typename SizeType  = std::size_t>
struct interval
{
private:
    typedef internal::interval_kernel<Type> kernel;
    typedef typename kernel::core_type      core_type;

    // the inclusive bounds and the kind, as constants: the conversion types
    // below name them as template arguments, so they come before them
    static D_CONSTEXPR_VAR Type m_eff_lower =
        LeftOpen ? internal::interval_bound_step<Type, Lower>::up : Lower;
    static D_CONSTEXPR_VAR Type m_eff_upper =
        RightOpen ? internal::interval_bound_step<Type, Upper>::down : Upper;
    static D_CONSTEXPR_VAR bool m_is_discrete = (static_cast<Type>(0) < Step);

public:
    typedef Type                         value_type;
    typedef SizeType                     size_type;
    typedef interval_iterator<interval>  iterator;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type upper_bound   = Upper;
    static D_CONSTEXPR_VAR value_type step          = Step;
    static D_CONSTEXPR_VAR bool       is_left_open  = LeftOpen;
    static D_CONSTEXPR_VAR bool       is_right_open = RightOpen;
    static D_CONSTEXPR_VAR bool       is_discrete   = m_is_discrete;

    D_STATIC_ASSERT((!(Upper < Lower)),
                    "interval: Lower must be <= Upper.");
    D_STATIC_ASSERT((!(Step < static_cast<Type>(0))),
                    "interval: Step must be >= 0 "
                    "(0 = continuous, > 0 = discrete).");

    // ---- conversion types --------------------------------------------------

    // as_closed
    //   type: this interval with both endpoints closed, over the inclusive
    // bounds: open (2, 8) becomes closed [3, 7].
    typedef interval<Type,
                     m_eff_lower,
                     m_eff_upper,
                     false,
                     false,
                     Step,
                     SizeType> as_closed;

    // as_open
    //   type: this interval with both endpoints open, the bounds widened one
    // unit outward: closed [3, 7] becomes open (2, 8).
    typedef interval<Type,
                     internal::interval_bound_step<Type, m_eff_lower>::down,
                     internal::interval_bound_step<Type, m_eff_upper>::up,
                     true,
                     true,
                     Step,
                     SizeType> as_open;

    // as_half_open_right
    //   type: this interval as [inclusive lower, inclusive upper + 1).
    typedef interval<Type,
                     m_eff_lower,
                     internal::interval_bound_step<Type, m_eff_upper>::up,
                     false,
                     true,
                     Step,
                     SizeType> as_half_open_right;

    // as_half_open_left
    //   type: this interval as (inclusive lower - 1, inclusive upper].
    typedef interval<Type,
                     internal::interval_bound_step<Type, m_eff_lower>::down,
                     m_eff_upper,
                     true,
                     false,
                     Step,
                     SizeType> as_half_open_left;

    // as_continuous
    //   type: this interval with its step removed.
    typedef interval<Type,
                     Lower,
                     Upper,
                     LeftOpen,
                     RightOpen,
                     static_cast<Type>(0),
                     SizeType> as_continuous;

    // to_closed_interval
    //   type: the equivalent closed_interval.
    typedef closed_interval<Type,
                            m_eff_lower,
                            m_eff_upper,
                            SizeType> to_closed_interval;

    // to_open_interval
    //   type: the equivalent open_interval.
    typedef open_interval<
                Type,
                internal::interval_bound_step<Type, m_eff_lower>::down,
                internal::interval_bound_step<Type, m_eff_upper>::up,
                SizeType> to_open_interval;

    // to_discrete_interval
    //   type: the equivalent discrete_interval: Step if discrete, else 1.
    typedef discrete_interval<Type,
                              m_eff_lower,
                              m_eff_upper,
                              (m_is_discrete ? Step
                                             : static_cast<Type>(1)),
                              SizeType> to_discrete_interval;

    // rebind_step
    //   trait: this interval with another step, as ::type (every level).
    template<Type NewStep>
    struct rebind_step
    {
        typedef interval<Type,
                         Lower,
                         Upper,
                         LeftOpen,
                         RightOpen,
                         NewStep,
                         SizeType> type;
    };

    // rebind_bounds
    //   trait: this interval with other bounds, as ::type (every level).
    template<Type NewLower,
             Type NewUpper>
    struct rebind_bounds
    {
        typedef interval<Type,
                         NewLower,
                         NewUpper,
                         LeftOpen,
                         RightOpen,
                         Step,
                         SizeType> type;
    };

    // rebind_size_type
    //   trait: this interval with another size type, as ::type (every level).
    template<typename NewSizeType>
    struct rebind_size_type
    {
        typedef interval<Type,
                         Lower,
                         Upper,
                         LeftOpen,
                         RightOpen,
                         Step,
                         NewSizeType> type;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // with_step / with_bounds / with_size_type
    //   type: the rebind_* traits' types, as alias templates (C++11).
    template<Type NewStep>
    using with_step = typename rebind_step<NewStep>::type;

    template<Type NewLower,
             Type NewUpper>
    using with_bounds = typename rebind_bounds<NewLower, NewUpper>::type;

    template<typename NewSizeType>
    using with_size_type = typename rebind_size_type<NewSizeType>::type;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // ---- size and emptiness ------------------------------------------------

    // size
    //   query: the number of values; step-aligned ones when discrete.
    static D_CONSTEXPR_CPP14 size_type
    size() D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::count(m_core()));
    }

    // is_empty
    //   query: whether the interval holds no value.
    static D_CONSTEXPR_CPP14 bool
    is_empty() D_NOEXCEPT
    {
        return kernel::is_empty(m_core());
    }

    // ---- containment -------------------------------------------------------

    // contains
    //   query: whether _value lies within the interval, respecting openness
    // and, when discrete, the stride from the first member.
    static D_CONSTEXPR_CPP14 bool
    contains(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains(m_core(), _value);
    }

    // contains_in_range
    //   query: whether _value lies within the bounds, on a step or not.
    static D_CONSTEXPR_CPP14 bool
    contains_in_range(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains_in_range(m_core(), _value);
    }

    // ---- clamping ----------------------------------------------------------

    // clamp
    //   transform: _value constrained to the interval; when discrete, down to
    // a whole step.
    static D_CONSTEXPR_CPP14 value_type
    clamp(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp(m_core(), _value);
    }

    // clamp_nearest
    //   transform: _value constrained to the nearer member, a tie going down;
    // clamp() for a continuous interval.
    static D_CONSTEXPR_CPP14 value_type
    clamp_nearest(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp_nearest(m_core(), _value);
    }

    // ---- normalization -----------------------------------------------------
    // Each maps _value to [0, 1], as a double or, as <FloatType>, in another
    // precision: normalize over [Lower, Upper], normalize_effective over the
    // inclusive bounds, normalize_discrete over the step indices.

    static D_CONSTEXPR_CPP14 double
    normalize(const value_type& _value) D_NOEXCEPT
    {
        return normalize<double>(_value);
    }

    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    normalize(const value_type& _value) D_NOEXCEPT
    {
        return m_divide<FloatType>(kernel::normalize_terms(m_core(), _value));
    }

    static D_CONSTEXPR_CPP14 double
    normalize_effective(const value_type& _value) D_NOEXCEPT
    {
        return normalize_effective<double>(_value);
    }

    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    normalize_effective(const value_type& _value) D_NOEXCEPT
    {
        return m_divide<FloatType>(
            kernel::normalize_effective_terms(m_core(), _value));
    }

    static D_CONSTEXPR_CPP14 double
    normalize_discrete(const value_type& _value) D_NOEXCEPT
    {
        return normalize_discrete<double>(_value);
    }

    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    normalize_discrete(const value_type& _value) D_NOEXCEPT
    {
        return m_divide<FloatType>(
            kernel::normalize_discrete_terms(m_core(), _value));
    }

    // ---- discrete access ---------------------------------------------------

    // at
    //   access: the member _index steps past the first (a continuous
    // interval steps by 1).
    static D_CONSTEXPR_CPP14 value_type
    at(size_type _index) D_NOEXCEPT
    {
        return kernel::at(m_core(), static_cast<d_math_umax>(_index));
    }

    // index_of
    //   query: _value's index, or size() if it is not a member.
    static D_CONSTEXPR_CPP14 size_type
    index_of(const value_type& _value) D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::index_of(m_core(), _value));
    }

    // first
    //   query: the smallest member.
    static D_CONSTEXPR_CPP14 value_type
    first() D_NOEXCEPT
    {
        return kernel::first(m_core());
    }

    // last
    //   query: the largest member; when discrete, the last whole step.
    static D_CONSTEXPR_CPP14 value_type
    last() D_NOEXCEPT
    {
        return kernel::last(m_core());
    }

    // ---- overlap detection -------------------------------------------------
    // Each compares inclusive ranges, ignoring strides; an empty interval
    // overlaps nothing.

    template<Type     OtherLower,
             Type     OtherUpper,
             bool     OtherLeftOpen,
             bool     OtherRightOpen,
             Type     OtherStep,
             typename OtherSizeType>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const interval<Type,
                       OtherLower,
                       OtherUpper,
                       OtherLeftOpen,
                       OtherRightOpen,
                       OtherStep,
                       OtherSizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(
            m_core(),
            kernel::make(OtherLower,
                         OtherUpper,
                         internal::interval_bounds(OtherLeftOpen,
                                                   OtherRightOpen),
                         OtherStep));
    }

    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const closed_interval<Type,
                              OtherLower,
                              OtherUpper,
                              SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(m_core(),
                                kernel::make(OtherLower,
                                             OtherUpper,
                                             D_INTERVAL_CLOSED,
                                             static_cast<Type>(0)));
    }

    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const open_interval<Type,
                            OtherLower,
                            OtherUpper,
                            SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(m_core(),
                                kernel::make(OtherLower,
                                             OtherUpper,
                                             D_INTERVAL_OPEN,
                                             static_cast<Type>(0)));
    }

    template<Type OtherLower,
             Type OtherUpper,
             Type OtherStep>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const discrete_interval<Type,
                                OtherLower,
                                OtherUpper,
                                OtherStep,
                                SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(m_core(),
                                kernel::make(OtherLower,
                                             OtherUpper,
                                             D_INTERVAL_CLOSED,
                                             OtherStep));
    }

    // ---- shared members ----------------------------------------------------
    // Each is true when the intervals share a member: a value both hold,
    // strides and openness considered -- for two discrete intervals over an
    // integer type, a value on both strides, by the Chinese remainder
    // theorem. Two continuous intervals share a member where they overlap.

    template<Type     OtherLower,
             Type     OtherUpper,
             bool     OtherLeftOpen,
             bool     OtherRightOpen,
             Type     OtherStep,
             typename OtherSizeType>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const interval<Type,
                       OtherLower,
                       OtherUpper,
                       OtherLeftOpen,
                       OtherRightOpen,
                       OtherStep,
                       OtherSizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(
            m_core(),
            kernel::make(OtherLower,
                         OtherUpper,
                         internal::interval_bounds(OtherLeftOpen,
                                                   OtherRightOpen),
                         OtherStep));
    }

    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const closed_interval<Type,
                              OtherLower,
                              OtherUpper,
                              SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_CLOSED,
                                               static_cast<Type>(0)));
    }

    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const open_interval<Type,
                            OtherLower,
                            OtherUpper,
                            SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_OPEN,
                                               static_cast<Type>(0)));
    }

    template<Type OtherLower,
             Type OtherUpper,
             Type OtherStep>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const discrete_interval<Type,
                                OtherLower,
                                OtherUpper,
                                OtherStep,
                                SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_CLOSED,
                                               OtherStep));
    }

    // ---- validation --------------------------------------------------------

    // is_valid
    //   query: whether the interval is well-formed (Lower <= Upper, Step >=
    // 0).
    static D_CONSTEXPR_CPP14 bool
    is_valid() D_NOEXCEPT
    {
        return kernel::is_valid(m_core());
    }

    // is_degenerate
    //   query: whether the interval holds exactly one value.
    static D_CONSTEXPR_CPP14 bool
    is_degenerate() D_NOEXCEPT
    {
        return (kernel::count(m_core()) == 1u);
    }

    // ---- string representation ---------------------------------------------

    // to_string
    //   format: "[a, b]", "(a, b)", "[a, b)", "(a, b]", or with the step,
    // "[a:s:b]".
    static std::string
    to_string()
    {
        return kernel::to_string(m_core());
    }

    // ---- iteration ---------------------------------------------------------

    // begin / end
    //   iteration: over the members, by index.
    static D_CONSTEXPR iterator
    begin() D_NOEXCEPT
    {
        return iterator(0);
    }

    static D_CONSTEXPR_CPP14 iterator
    end() D_NOEXCEPT
    {
        return iterator(size());
    }

private:
    // m_core
    //   the interval as its kernel takes it.
    static D_CONSTEXPR_CPP14 core_type
    m_core() D_NOEXCEPT
    {
        return kernel::make(Lower,
                            Upper,
                            internal::interval_bounds(LeftOpen, RightOpen),
                            Step);
    }

    // m_divide
    //   a normalization's terms divided in FloatType.
    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    m_divide(const struct d_math_ratio& _terms) D_NOEXCEPT
    {
        return ( static_cast<FloatType>(_terms.numerator) /
                 static_cast<FloatType>(_terms.denominator) );
    }
};

// 1.2    Static member definitions
//------------------------------------------------------------------------------
// As closed_interval.hpp's: definitions below C++17, redeclarations from it.
#define D_INTERNAL_MATH_INTERVAL_STATIC(TYPE, NAME)                            \
    template<typename Type,                                                    \
             Type     Lower,                                                   \
             Type     Upper,                                                   \
             bool     LeftOpen,                                                \
             bool     RightOpen,                                               \
             Type     Step,                                                    \
             typename SizeType>                                                \
    D_CONSTEXPR_VAR TYPE interval<Type,                                        \
                                  Lower,                                       \
                                  Upper,                                       \
                                  LeftOpen,                                    \
                                  RightOpen,                                   \
                                  Step,                                        \
                                  SizeType>::NAME;

D_INTERNAL_MATH_INTERVAL_STATIC(Type, m_eff_lower)
D_INTERNAL_MATH_INTERVAL_STATIC(Type, m_eff_upper)
D_INTERNAL_MATH_INTERVAL_STATIC(bool, m_is_discrete)
D_INTERNAL_MATH_INTERVAL_STATIC(Type, lower_bound)
D_INTERNAL_MATH_INTERVAL_STATIC(Type, upper_bound)
D_INTERNAL_MATH_INTERVAL_STATIC(Type, step)
D_INTERNAL_MATH_INTERVAL_STATIC(bool, is_left_open)
D_INTERNAL_MATH_INTERVAL_STATIC(bool, is_right_open)
D_INTERNAL_MATH_INTERVAL_STATIC(bool, is_discrete)

#undef D_INTERNAL_MATH_INTERVAL_STATIC


//==============================================================================
// 2.  INTER-TYPE CONVERSION METAFUNCTIONS
//==============================================================================
// Each converts any interval-like type -- one with value_type, size_type and
// the structural constants -- to one of the four templates, as ::type.


NS_INTERNAL

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // interval_cast_helper
    //   helper: primary template, declared and never defined (C++17: `auto`
    // template parameters).
    template<template<typename, auto, auto, auto...> typename Target,
             typename                                         Source>
    struct interval_cast_helper;
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    // interval_cast_to_closed
    //   helper: converts any interval to closed_interval.
    template<typename Source>
    struct interval_cast_to_closed
    {
    private:
        typedef typename Source::value_type vt;
        typedef typename Source::size_type  st;

        static D_CONSTEXPR_VAR vt eff_lo =
            Source::is_left_open
                ? interval_bound_step<vt, Source::lower_bound>::up
                : Source::lower_bound;
        static D_CONSTEXPR_VAR vt eff_hi =
            Source::is_right_open
                ? interval_bound_step<vt, Source::upper_bound>::down
                : Source::upper_bound;

    public:
        typedef closed_interval<vt, eff_lo, eff_hi, st> type;
    };

    // interval_cast_to_open
    //   helper: converts any interval to open_interval.
    template<typename Source>
    struct interval_cast_to_open
    {
    private:
        typedef typename Source::value_type vt;
        typedef typename Source::size_type  st;

        static D_CONSTEXPR_VAR vt eff_lo =
            Source::is_left_open
                ? interval_bound_step<vt, Source::lower_bound>::up
                : Source::lower_bound;
        static D_CONSTEXPR_VAR vt eff_hi =
            Source::is_right_open
                ? interval_bound_step<vt, Source::upper_bound>::down
                : Source::upper_bound;

    public:
        typedef open_interval<vt,
                              interval_bound_step<vt, eff_lo>::down,
                              interval_bound_step<vt, eff_hi>::up,
                              st> type;
    };

    // interval_cast_to_discrete
    //   helper: converts any interval to discrete_interval, keeping a
    // positive step and otherwise using 1.
    template<typename Source>
    struct interval_cast_to_discrete
    {
    private:
        typedef typename Source::value_type vt;
        typedef typename Source::size_type  st;

        static D_CONSTEXPR_VAR vt eff_lo =
            Source::is_left_open
                ? interval_bound_step<vt, Source::lower_bound>::up
                : Source::lower_bound;
        static D_CONSTEXPR_VAR vt eff_hi =
            Source::is_right_open
                ? interval_bound_step<vt, Source::upper_bound>::down
                : Source::upper_bound;
        static D_CONSTEXPR_VAR vt resolved_step =
            (static_cast<vt>(0) < Source::step) ? Source::step
                                                : static_cast<vt>(1);

    public:
        typedef discrete_interval<vt, eff_lo, eff_hi, resolved_step, st> type;
    };

    // interval_cast_to_interval
    //   helper: converts any interval to the unified interval, keeping every
    // property.
    template<typename Source>
    struct interval_cast_to_interval
    {
    private:
        typedef typename Source::value_type vt;
        typedef typename Source::size_type  st;

    public:
        typedef interval<vt,
                         Source::lower_bound,
                         Source::upper_bound,
                         Source::is_left_open,
                         Source::is_right_open,
                         Source::step,
                         st> type;
    };

NS_END  // internal

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// to_closed_interval_t
//   type: converts any interval-like type to closed_interval.
template<typename Source>
using to_closed_interval_t =
    typename internal::interval_cast_to_closed<Source>::type;

// to_open_interval_t
//   type: converts any interval-like type to open_interval.
template<typename Source>
using to_open_interval_t =
    typename internal::interval_cast_to_open<Source>::type;

// to_discrete_interval_t
//   type: converts any interval-like type to discrete_interval.
template<typename Source>
using to_discrete_interval_t =
    typename internal::interval_cast_to_discrete<Source>::type;

// to_interval_t
//   type: converts any interval-like type to the unified interval.
template<typename Source>
using to_interval_t =
    typename internal::interval_cast_to_interval<Source>::type;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 3.  CONVENIENCE TYPE ALIASES (C++11)
//==============================================================================
// Alias templates are C++11's; below it, spell interval<int, L, U, ...>.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- closed (default) -------------------------------------------------------

// int_interval
//   type: closed continuous interval over int.
template<int Lower,
         int Upper>
using int_interval = interval<int, Lower, Upper>;

// index_interval
//   type: closed continuous interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper>
using index_interval = interval<std::size_t, Lower, Upper>;

// char_interval
//   type: closed continuous interval over char.
template<char Lower,
         char Upper>
using char_interval = interval<char, Lower, Upper>;

// uint8_interval
//   type: closed continuous interval over uint8_t.
template<re_std::uint8_t Lower,
         re_std::uint8_t Upper>
using uint8_interval = interval<re_std::uint8_t, Lower, Upper>;

// int8_interval
//   type: closed continuous interval over int8_t.
template<re_std::int8_t Lower,
         re_std::int8_t Upper>
using int8_interval = interval<re_std::int8_t, Lower, Upper>;

// uint16_interval
//   type: closed continuous interval over uint16_t.
template<re_std::uint16_t Lower,
         re_std::uint16_t Upper>
using uint16_interval = interval<re_std::uint16_t, Lower, Upper>;

// int16_interval
//   type: closed continuous interval over int16_t.
template<re_std::int16_t Lower,
         re_std::int16_t Upper>
using int16_interval = interval<re_std::int16_t, Lower, Upper>;

// uint32_interval
//   type: closed continuous interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper>
using uint32_interval = interval<re_std::uint32_t, Lower, Upper>;

// int32_interval
//   type: closed continuous interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper>
using int32_interval = interval<re_std::int32_t, Lower, Upper>;

// uint64_interval
//   type: closed continuous interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper>
using uint64_interval = interval<re_std::uint64_t, Lower, Upper>;

// int64_interval
//   type: closed continuous interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper>
using int64_interval = interval<re_std::int64_t, Lower, Upper>;

// short_interval
//   type: closed continuous interval over short.
template<short Lower,
         short Upper>
using short_interval = interval<short, Lower, Upper>;

// long_interval
//   type: closed continuous interval over long.
template<long Lower,
         long Upper>
using long_interval = interval<long, Lower, Upper>;

// long_long_interval
//   type: closed continuous interval over long long.
template<long long Lower,
         long long Upper>
using long_long_interval = interval<long long, Lower, Upper>;

// bool_interval
//   type: closed continuous interval over bool.
template<bool Lower,
         bool Upper>
using bool_interval = interval<bool, Lower, Upper>;

// --- open -------------------------------------------------------------------

// int_open
//   type: open continuous interval over int.
template<int Lower,
         int Upper>
using int_open = interval<int, Lower, Upper, true, true>;

// index_open
//   type: open continuous interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper>
using index_open = interval<std::size_t, Lower, Upper, true, true>;

// char_open
//   type: open continuous interval over char.
template<char Lower,
         char Upper>
using char_open = interval<char, Lower, Upper, true, true>;

// int32_open
//   type: open continuous interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper>
using int32_open = interval<re_std::int32_t, Lower, Upper, true, true>;

// uint32_open
//   type: open continuous interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper>
using uint32_open =
    interval<re_std::uint32_t, Lower, Upper, true, true>;

// int64_open
//   type: open continuous interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper>
using int64_open =
    interval<re_std::int64_t, Lower, Upper, true, true>;

// uint64_open
//   type: open continuous interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper>
using uint64_open =
    interval<re_std::uint64_t, Lower, Upper, true, true>;

// --- half-open [a, b) -------------------------------------------------------

// int_half_open
//   type: half-open-right continuous interval over int.
template<int Lower,
         int Upper>
using int_half_open = interval<int, Lower, Upper, false, true>;

// index_half_open
//   type: half-open-right continuous interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper>
using index_half_open =
    interval<std::size_t, Lower, Upper, false, true>;

// int32_half_open
//   type: half-open-right continuous interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper>
using int32_half_open =
    interval<re_std::int32_t, Lower, Upper, false, true>;

// int64_half_open
//   type: half-open-right continuous interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper>
using int64_half_open =
    interval<re_std::int64_t, Lower, Upper, false, true>;

// --- discrete ---------------------------------------------------------------

// int_stepped
//   type: closed discrete interval over int.
template<int Lower,
         int Upper,
         int Step = 1>
using int_stepped =
    interval<int, Lower, Upper, false, false, Step>;

// index_stepped
//   type: closed discrete interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper,
         std::size_t Step = 1>
using index_stepped =
    interval<std::size_t, Lower, Upper, false, false, Step>;

// int32_stepped
//   type: closed discrete interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper,
         re_std::int32_t Step = 1>
using int32_stepped =
    interval<re_std::int32_t, Lower, Upper, false, false, Step>;

// int64_stepped
//   type: closed discrete interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper,
         re_std::int64_t Step = 1>
using int64_stepped =
    interval<re_std::int64_t, Lower, Upper, false, false, Step>;

// uint32_stepped
//   type: closed discrete interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper,
         re_std::uint32_t Step = 1>
using uint32_stepped =
    interval<re_std::uint32_t, Lower, Upper, false, false, Step>;

// uint64_stepped
//   type: closed discrete interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper,
         re_std::uint64_t Step = 1>
using uint64_stepped =
    interval<re_std::uint64_t, Lower, Upper, false, false, Step>;

// char_stepped
//   type: closed discrete interval over char.
template<char Lower,
         char Upper,
         char Step = 1>
using char_stepped =
    interval<char, Lower, Upper, false, false, Step>;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 4.  INTERVAL TRAITS
//==============================================================================
// Structural detection of interval types and their properties, at every
// level: presence by core/meta's detection engine (its sizeof engine below
// C++11), then a pointer probe for what presence cannot tell -- that a
// member is static, and of type bool -- which gives the same answer at every
// level, a non-static member counting as absent.


// 4.1    Detection helpers
//------------------------------------------------------------------------------
NS_INTERNAL

    // interval_probe_yes / interval_probe_no
    //   type: the two answers a probe can give, told apart by size.
    typedef char interval_probe_yes;

    struct interval_probe_no
    {
        char answer[2];
    };

    // interval_static_probe / interval_bool_probe
    //   function: declared only, for sizeof: a pointer to an object (or to a
    // function) answers yes, a pointer to member no; the bool probe answers
    // yes only for a pointer to a bool.
    template<typename Pointee>
    interval_probe_yes interval_static_probe(Pointee*);
    interval_probe_no  interval_static_probe(...);
    interval_probe_yes interval_bool_probe(const bool*);
    interval_probe_no  interval_bool_probe(...);

    D_TYPE_TRAIT_HAS_STATIC_MEMBER(interval_names_lower_bound, lower_bound)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(interval_names_upper_bound, upper_bound)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(interval_names_is_left_open, is_left_open)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(interval_names_is_right_open,
                                   is_right_open)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(interval_names_step, step)
    D_TYPE_TRAIT_HAS_TYPE(interval_names_value_type, value_type)
    D_TYPE_TRAIT_HAS_TYPE(interval_names_size_type, size_type)

    // has_lower_bound
    //   helper: detects a static lower_bound member.
    template<typename Type,
             bool     Named = interval_names_lower_bound<Type>::value>
    struct has_lower_bound
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_lower_bound<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(interval_static_probe(&Type::lower_bound)) ==
                sizeof(interval_probe_yes) )>
    {};

    // has_upper_bound
    //   helper: detects a static upper_bound member.
    template<typename Type,
             bool     Named = interval_names_upper_bound<Type>::value>
    struct has_upper_bound
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_upper_bound<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(interval_static_probe(&Type::upper_bound)) ==
                sizeof(interval_probe_yes) )>
    {};

    // has_is_left_open
    //   helper: detects a static bool is_left_open member.
    template<typename Type,
             bool     Named = interval_names_is_left_open<Type>::value>
    struct has_is_left_open
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_is_left_open<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(interval_bool_probe(&Type::is_left_open)) ==
                sizeof(interval_probe_yes) )>
    {};

    // has_is_right_open
    //   helper: detects a static bool is_right_open member.
    template<typename Type,
             bool     Named = interval_names_is_right_open<Type>::value>
    struct has_is_right_open
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_is_right_open<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(interval_bool_probe(&Type::is_right_open)) ==
                sizeof(interval_probe_yes) )>
    {};

    // has_step
    //   helper: detects a static step member (present on every interval
    // type; a zero step is a continuous interval, so presence alone does not
    // mean discreteness -- see has_discrete_step).
    template<typename Type,
             bool     Named = interval_names_step<Type>::value>
    struct has_step
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_step<Type, true>
        : re_std::integral_constant<bool,
              ( sizeof(interval_static_probe(&Type::step)) ==
                sizeof(interval_probe_yes) )>
    {};

    // has_discrete_step
    //   helper: detects a static step member whose value is positive: what
    // tells a discrete interval from a continuous one.
    template<typename Type,
             bool     Stepped = has_step<Type>::value>
    struct has_discrete_step
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct has_discrete_step<Type, true>
        : re_std::integral_constant<bool, (Type::step > 0)>
    {};

    // interval_structural_check
    //   helper: every structural requirement of an interval type.
    template<typename Type>
    struct interval_structural_check
        : re_std::integral_constant<bool,
              ( has_lower_bound<Type>::value  &&
                has_upper_bound<Type>::value  &&
                has_is_left_open<Type>::value &&
                has_is_right_open<Type>::value )>
    {};

    // discrete_interval_structural_check
    //   helper: the structural requirements of a discrete interval type.
    template<typename Type>
    struct discrete_interval_structural_check
        : re_std::integral_constant<bool,
              ( interval_structural_check<Type>::value &&
                has_discrete_step<Type>::value )>
    {};

NS_END  // internal

// 4.2    Interval detection
//------------------------------------------------------------------------------

// is_interval
//   trait: whether Type is an interval type: static lower_bound and
// upper_bound members, and static bool is_left_open and is_right_open.
template<typename Type,
         typename Enable = void>
struct is_interval
    : re_std::integral_constant<bool,
          internal::interval_structural_check<Type>::value>
{};

// is_discrete_interval
//   trait: whether Type is an interval type with a positive step.
template<typename Type,
         typename Enable = void>
struct is_discrete_interval
    : re_std::integral_constant<bool,
          internal::discrete_interval_structural_check<Type>::value>
{};

// is_continuous_interval
//   trait: whether Type is an interval type without a positive step.
template<typename Type,
         typename Enable = void>
struct is_continuous_interval
    : re_std::integral_constant<bool,
          ( is_interval<Type>::value &&
            !is_discrete_interval<Type>::value )>
{};

// 4.3    Boundary type detection
//------------------------------------------------------------------------------
NS_INTERNAL

    // is_closed_check / is_open_check / is_half_open_check
    //   helper: the boundary conditions, for an interval type.
    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct is_closed_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct is_closed_check<Type, true>
        : re_std::integral_constant<bool,
              ( (!Type::is_left_open) &&
                (!Type::is_right_open) )>
    {};

    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct is_open_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct is_open_check<Type, true>
        : re_std::integral_constant<bool,
              ( (Type::is_left_open) &&
                (Type::is_right_open) )>
    {};

    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct is_half_open_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct is_half_open_check<Type, true>
        : re_std::integral_constant<bool,
              (Type::is_left_open != Type::is_right_open)>
    {};

NS_END  // internal

// is_closed
//   trait: whether an interval has both endpoints closed (inclusive).
template<typename Type>
struct is_closed : internal::is_closed_check<Type>
{};

// is_open
//   trait: whether an interval has both endpoints open (exclusive).
template<typename Type>
struct is_open : internal::is_open_check<Type>
{};

// is_half_open
//   trait: whether an interval has exactly one open endpoint.
template<typename Type>
struct is_half_open : internal::is_half_open_check<Type>
{};

// 4.4    Endpoint detection
//------------------------------------------------------------------------------
NS_INTERNAL

    // left_open_check / right_open_check
    //   helper: whether an interval type's left (right) endpoint is open.
    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct left_open_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct left_open_check<Type, true>
        : re_std::integral_constant<bool, Type::is_left_open>
    {};

    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct right_open_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct right_open_check<Type, true>
        : re_std::integral_constant<bool, Type::is_right_open>
    {};

    // left_closed_check / right_closed_check
    //   helper: whether an interval type's left (right) endpoint is closed.
    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct left_closed_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct left_closed_check<Type, true>
        : re_std::integral_constant<bool, (!Type::is_left_open)>
    {};

    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct right_closed_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct right_closed_check<Type, true>
        : re_std::integral_constant<bool, (!Type::is_right_open)>
    {};

NS_END  // internal

// is_left_open
//   trait: whether an interval has its left endpoint open.
template<typename Type>
struct is_left_open : internal::left_open_check<Type>
{};

// is_right_open
//   trait: whether an interval has its right endpoint open.
template<typename Type>
struct is_right_open : internal::right_open_check<Type>
{};

// is_left_closed
//   trait: whether an interval has its left endpoint closed.
template<typename Type>
struct is_left_closed : internal::left_closed_check<Type>
{};

// is_right_closed
//   trait: whether an interval has its right endpoint closed.
template<typename Type>
struct is_right_closed : internal::right_closed_check<Type>
{};

// 4.5    Interval property detection
//------------------------------------------------------------------------------

// is_bounded_interval
//   trait: whether an interval has finite bounds: every compile-time
// interval, whose bounds are template arguments.
template<typename Type>
struct is_bounded_interval : is_interval<Type>
{};

NS_INTERNAL

    // empty_interval_check
    //   helper: whether an interval type is empty by its bounds: lower >
    // upper, or lower == upper with either endpoint open.
    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct empty_interval_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct empty_interval_check<Type, true>
        : re_std::integral_constant<bool,
              ( (Type::upper_bound < Type::lower_bound)        ||
                ( (!(Type::lower_bound < Type::upper_bound)) &&
                  (Type::is_left_open || Type::is_right_open) ) )>
    {};

    // degenerate_interval_check
    //   helper: whether an interval type holds exactly one element by its
    // bounds: lower == upper with both endpoints closed.
    template<typename Type,
             bool     IsInterval = is_interval<Type>::value>
    struct degenerate_interval_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct degenerate_interval_check<Type, true>
        : re_std::integral_constant<bool,
              ( (!(Type::lower_bound < Type::upper_bound)) &&
                (!(Type::upper_bound < Type::lower_bound)) &&
                (!Type::is_left_open)                      &&
                (!Type::is_right_open) )>
    {};

NS_END  // internal

// is_empty_interval
//   trait: whether an interval contains no elements, by its bounds.
template<typename Type>
struct is_empty_interval : internal::empty_interval_check<Type>
{};

// is_degenerate_interval
//   trait: whether an interval contains exactly one element, by its bounds.
template<typename Type>
struct is_degenerate_interval
    : internal::degenerate_interval_check<Type>
{};

// is_proper_interval
//   trait: whether an interval type is non-empty.
template<typename Type>
struct is_proper_interval
    : re_std::integral_constant<bool,
          ( is_interval<Type>::value &&
            !is_empty_interval<Type>::value )>
{};

// 4.6    Interval type extraction
//------------------------------------------------------------------------------
NS_INTERNAL

    // interval_value_type_helper / interval_size_type_helper
    //   helper: Type's value_type (size_type) if it has one, else void.
    template<typename Type,
             bool     Named = interval_names_value_type<Type>::value>
    struct interval_value_type_helper
    {
        typedef void type;
    };

    template<typename Type>
    struct interval_value_type_helper<Type, true>
    {
        typedef typename Type::value_type type;
    };

    template<typename Type,
             bool     Named = interval_names_size_type<Type>::value>
    struct interval_size_type_helper
    {
        typedef void type;
    };

    template<typename Type>
    struct interval_size_type_helper<Type, true>
    {
        typedef typename Type::size_type type;
    };

NS_END  // internal

// interval_value_type
//   trait: the value type of an interval type, or void.
template<typename Type>
struct interval_value_type
{
    typedef typename internal::interval_value_type_helper<Type>::type type;
};

// interval_size_type
//   trait: the size type of an interval type, or void.
template<typename Type>
struct interval_size_type
{
    typedef typename internal::interval_size_type_helper<Type>::type type;
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// interval_value_type_t
//   type: shorthand for interval_value_type<Type>::type.
template<typename Type>
using interval_value_type_t =
    typename interval_value_type<Type>::type;

// interval_size_type_t
//   type: shorthand for interval_size_type<Type>::type.
template<typename Type>
using interval_size_type_t =
    typename interval_size_type<Type>::type;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 4.7    Interval relationship traits
//------------------------------------------------------------------------------
NS_INTERNAL

    // intervals_same_type_check
    //   helper: whether two interval types have the same value type.
    template<typename Interval1,
             typename Interval2,
             bool     BothIntervals = ( is_interval<Interval1>::value &&
                                        is_interval<Interval2>::value )>
    struct intervals_same_type_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Interval1,
             typename Interval2>
    struct intervals_same_type_check<Interval1, Interval2, true>
        : re_std::integral_constant<bool,
              re_std::is_same<
                  typename interval_value_type<Interval1>::type,
                  typename interval_value_type<Interval2>::type>::value>
    {};

    // intervals_same_boundary_check
    //   helper: whether two interval types have the same boundary kind.
    template<typename Interval1,
             typename Interval2,
             bool     BothIntervals = ( is_interval<Interval1>::value &&
                                        is_interval<Interval2>::value )>
    struct intervals_same_boundary_check
        : re_std::integral_constant<bool, false>
    {};

    template<typename Interval1,
             typename Interval2>
    struct intervals_same_boundary_check<Interval1, Interval2, true>
        : re_std::integral_constant<bool,
              ( (Interval1::is_left_open  == Interval2::is_left_open) &&
                (Interval1::is_right_open == Interval2::is_right_open) )>
    {};

NS_END  // internal

// intervals_same_type
//   trait: whether two intervals have the same value type.
template<typename Interval1,
         typename Interval2>
struct intervals_same_type
    : internal::intervals_same_type_check<Interval1, Interval2>
{};

// intervals_same_boundary_type
//   trait: whether two intervals have the same boundary kind (open/closed).
template<typename Interval1,
         typename Interval2>
struct intervals_same_boundary_type
    : internal::intervals_same_boundary_check<Interval1, Interval2>
{};

// 4.8    Variable templates (C++14)
//------------------------------------------------------------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_interval_v ... intervals_same_boundary_type_v
    //   variable template: the value of each trait above.
    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_interval_v =
        is_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_discrete_interval_v =
        is_discrete_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_continuous_interval_v =
        is_continuous_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_closed_v =
        is_closed<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_open_v =
        is_open<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_half_open_v =
        is_half_open<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_left_open_v =
        is_left_open<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_right_open_v =
        is_right_open<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_left_closed_v =
        is_left_closed<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_right_closed_v =
        is_right_closed<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_bounded_interval_v =
        is_bounded_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_empty_interval_v =
        is_empty_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_degenerate_interval_v =
        is_degenerate_interval<Type>::value;

    template<typename Type>
    D_INLINE_VAR D_CONSTEXPR_VAR bool is_proper_interval_v =
        is_proper_interval<Type>::value;

    template<typename Interval1,
             typename Interval2>
    D_INLINE_VAR D_CONSTEXPR_VAR bool intervals_same_type_v =
        intervals_same_type<Interval1, Interval2>::value;

    template<typename Interval1,
             typename Interval2>
    D_INLINE_VAR D_CONSTEXPR_VAR bool intervals_same_boundary_type_v =
        intervals_same_boundary_type<Interval1, Interval2>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_INTERVAL_HPP
