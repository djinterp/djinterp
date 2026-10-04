/******************************************************************************
* djinterp [re_std]                                                  numbers.hpp
*
* numbers header:
*   The thirteen mathematical constants, each as a variable template
* over the floating-point type plus a double-typed shorthand:
*
*     numbers::pi_v<float>    numbers::pi_v<long double>
*     numbers::pi             (== pi_v<double>)
*
*   THE C++14 FLOOR IS NOT NEGOTIABLE HERE:
*   Nearly every re_std header reaches the C++11 floor. This one cannot,
* because the INTERFACE is a variable template and variable templates
* arrive in C++14. There is no back-port that preserves the spelling --
* a struct-of-static-members would be a different API, so offering one
* would mean inventing surface the standard does not have. The header
* gates itself out below C++14 rather than pretend.
*
*   NON-FLOATING-POINT TYPES ARE REJECTED, NOT COERCED:
*   [numbers.syn] leaves the primary template undefined and defines only
* the floating-point specialisation, so pi_v<int> must be ill-formed --
* not silently 3. A variable template cannot hold a static_assert, so
* the check is routed through internal::numbers_check, which gives a
* readable diagnostic at the point of use.
*
*   PRECISION:
*   Every literal is written with an L suffix and ~37 significant
* digits, which is enough to fill an 80-bit or 128-bit long double
* exactly. Writing them as double literals and converting up would lose
* the tail, and the loss would be invisible -- pi_v<long double> would
* silently carry only double precision.
*
*   PORTABILITY:
*   std added <numbers> in C++20 as constexpr; re_std back-ports to
* C++14, the earliest tier the interface can exist on.
*
*
* path:      /inc/djinterp/re_std/numbers/numbers.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_NUMBERS_NUMBERS_
#define DJINTERP_RE_STD_NUMBERS_NUMBERS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// djinterp
#include "../type_traits/is_floating_point.hpp"


NS_RESTD

// std places every one of these in a NESTED namespace -- they are
// std::numbers::pi, not std::pi -- so re_std mirrors it as
// re_std::numbers::pi. There is no NS_ macro for a project-specific
// nested namespace, so it is opened literally.
namespace numbers
{

// ===========================================================================
// I.   INTERNAL: THE FLOATING-POINT CONSTRAINT
// ===========================================================================

NS_INTERNAL

    // numbers_check
    //   helper: the standard leaves the primary variable template
    // UNDEFINED and only defines the floating-point specialisation, so
    // numbers::pi_v<int> is ill-formed rather than silently 3. A variable
    // template cannot carry a static_assert, so the check is routed
    // through this struct: instantiating it with a non-floating-point
    // type fires the assert with a readable message instead of a
    // deduction failure deep in the caller.
    template<typename _T>
    struct numbers_check
    {
        static_assert(is_floating_point<_T>::value,
            "re_std::numbers: the mathematical constants are defined only "
            "for floating-point types");

        static D_CONSTEXPR _T value(_T _v) { return _v; }
    };

NS_END  // internal


// ===========================================================================
// II.  THE CONSTANTS
// ===========================================================================


// e_v / e
//   constant: Euler's number.
template<typename _T>
D_CONSTEXPR _T e_v = internal::numbers_check<_T>::value(
    static_cast<_T>(2.718281828459045235360287471352662498L));

D_CONSTEXPR double e = e_v<double>;


// log2e_v / log2e
//   constant: log2(e).
template<typename _T>
D_CONSTEXPR _T log2e_v = internal::numbers_check<_T>::value(
    static_cast<_T>(1.442695040888963407359924681001892137L));

D_CONSTEXPR double log2e = log2e_v<double>;


// log10e_v / log10e
//   constant: log10(e).
template<typename _T>
D_CONSTEXPR _T log10e_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.434294481903251827651128918916605082L));

D_CONSTEXPR double log10e = log10e_v<double>;


// pi_v / pi
//   constant: pi.
template<typename _T>
D_CONSTEXPR _T pi_v = internal::numbers_check<_T>::value(
    static_cast<_T>(3.141592653589793238462643383279502884L));

D_CONSTEXPR double pi = pi_v<double>;


// inv_pi_v / inv_pi
//   constant: 1/pi.
template<typename _T>
D_CONSTEXPR _T inv_pi_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.318309886183790671537767526745028724L));

D_CONSTEXPR double inv_pi = inv_pi_v<double>;


// inv_sqrtpi_v / inv_sqrtpi
//   constant: 1/sqrt(pi).
template<typename _T>
D_CONSTEXPR _T inv_sqrtpi_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.564189583547756286948079451560772586L));

D_CONSTEXPR double inv_sqrtpi = inv_sqrtpi_v<double>;


// ln2_v / ln2
//   constant: ln(2).
template<typename _T>
D_CONSTEXPR _T ln2_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.693147180559945309417232121458176568L));

D_CONSTEXPR double ln2 = ln2_v<double>;


// ln10_v / ln10
//   constant: ln(10).
template<typename _T>
D_CONSTEXPR _T ln10_v = internal::numbers_check<_T>::value(
    static_cast<_T>(2.302585092994045684017991454684364208L));

D_CONSTEXPR double ln10 = ln10_v<double>;


// sqrt2_v / sqrt2
//   constant: sqrt(2).
template<typename _T>
D_CONSTEXPR _T sqrt2_v = internal::numbers_check<_T>::value(
    static_cast<_T>(1.414213562373095048801688724209698079L));

D_CONSTEXPR double sqrt2 = sqrt2_v<double>;


// sqrt3_v / sqrt3
//   constant: sqrt(3).
template<typename _T>
D_CONSTEXPR _T sqrt3_v = internal::numbers_check<_T>::value(
    static_cast<_T>(1.732050807568877293527446341505872367L));

D_CONSTEXPR double sqrt3 = sqrt3_v<double>;


// inv_sqrt3_v / inv_sqrt3
//   constant: 1/sqrt(3).
template<typename _T>
D_CONSTEXPR _T inv_sqrt3_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.577350269189625764509148780501957456L));

D_CONSTEXPR double inv_sqrt3 = inv_sqrt3_v<double>;


// egamma_v / egamma
//   constant: Euler-Mascheroni.
template<typename _T>
D_CONSTEXPR _T egamma_v = internal::numbers_check<_T>::value(
    static_cast<_T>(0.577215664901532860606512090082402431L));

D_CONSTEXPR double egamma = egamma_v<double>;


// phi_v / phi
//   constant: golden ratio.
template<typename _T>
D_CONSTEXPR _T phi_v = internal::numbers_check<_T>::value(
    static_cast<_T>(1.618033988749894848204586834365638118L));

D_CONSTEXPR double phi = phi_v<double>;


}  // namespace numbers

NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#endif  // DJINTERP_RE_STD_NUMBERS_NUMBERS_
