/******************************************************************************
* djinterp [math]                                            linalg_common.hpp
*
* Foundation of the linear-algebra subframework.
*   Single place for the things every linalg header needs: the nested
* djinterp::math::linalg namespace, forward declarations of vector / matrix,
* the structural trait families (is_vector / is_matrix / is_square_matrix with
* their _v and concept parallels), a small self-contained constexpr scalar
* kernel (sqrt / abs / acos) so norms and angles stay compile-time, and the
* default comparison tolerance.
*
* NAMESPACE NOTE:
*   Unlike the geometry subframework, which is flat in djinterp::math, the
* linear-algebra types live one level deeper in djinterp::math::linalg. This is
* deliberate: `vector`, `matrix`, `transpose`, and especially `dot` (already
* defined in expression.hpp) would otherwise collide in the shared math
* namespace. Pull them in with `using namespace djinterp::math::linalg;` or
* qualify as `linalg::vector<...>`.
*
* DUAL-API NOTE:
*   Every operation is exposed twice: as a const member returning a new value
* (so it chains, `a.transposed().times(a).trace()`) and as a free function in
* the linalg namespace (so it reads procedurally, `trace(times(transpose(a),
* a))`). The free functions are thin and delegate to the members; the two
* spellings always compute the same thing.
*
* path:      /inc/djinterp/math/linear_algebra/linalg_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_COMMON_
#define DJINTERP_MATH_LINALG_COMMON_ 1

// std
#include <cstddef>
#include <array>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"


// D_INLINE_VAR: `inline` for namespace-scope constexpr variables under C++17+;
// empty under C++14 (such a variable already has internal linkage). Replicated
// here so this header carries no dependency on expression.hpp.
#ifndef D_INLINE_VAR
    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        #define D_INLINE_VAR inline
    #else
        #define D_INLINE_VAR
    #endif
#endif


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   FORWARD DECLARATIONS
// ===========================================================================

// vector
//   class: fixed-size column vector (defined in vector.hpp).
template<typename    _T,
         std::size_t _N>
class vector;

// matrix
//   class: fixed-size, row-major matrix (defined in matrix.hpp).
template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
class matrix;


// ===========================================================================
// II.  CONSTEXPR SCALAR KERNEL  (internal)
// ===========================================================================
// Self-contained compile-time fallbacks so vector norms, matrix norms, and
// angles are usable in a constant-expression context (these mirror the kernel
// in expression.hpp but are kept local to keep the subframework independent).

NS_INTERNAL

    // abs_c
    //   function: absolute value of a scalar.
    template<typename _T>
    inline D_CONSTEXPR _T
    abs_c(_T _x) noexcept
    {
        return (_x < static_cast<_T>(0)) ? -_x : _x;
    }

    // sqrt_c
    //   function: square root by Newton-Raphson (non-positive input -> 0).
    inline D_CONSTEXPR double
    sqrt_c(double _x) noexcept
    {
        if (_x <= 0.0)
        {
            return 0.0;
        }

        double g = _x;

        // fixed iteration count keeps the body constexpr and branch-free.
        for (int i = 0; i < 100; ++i)
        {
            g = 0.5 * (g + _x / g);
        }

        return g;
    }

    // acos_c
    //   function: arc-cosine over [-1, 1], in radians. Uses the standard
    // sqrt-times-polynomial approximation (max abs error ~6.7e-5), which is
    // ample for angles and stays constexpr. Out-of-range inputs are clamped.
    inline D_CONSTEXPR double
    acos_c(double _x) noexcept
    {
        double x = _x;

        // clamp into the valid domain
        if (x < -1.0)
        {
            x = -1.0;
        }

        if (x > 1.0)
        {
            x = 1.0;
        }

        const bool   neg = (x < 0.0);
        const double a   = neg ? -x : x;            // a in [0, 1]

        const double poly =
            ( ( (-0.0187293 * a + 0.0742610) * a - 0.2121144 ) * a
              + 1.5707288 );

        const double r = poly * sqrt_c(1.0 - a);    // == acos(a), a in [0, 1]

        return neg ? (3.14159265358979323846 - r) : r;
    }

    // cos_c
    //   function: cosine, via range reduction to [-pi, pi] and a Taylor
    // series. Accurate to roughly 1e-13 across the reduced range.
    inline D_CONSTEXPR double
    cos_c(double _x) noexcept
    {
        const double pi     = 3.14159265358979323846;
        const double two_pi = 6.28318530717958647692;

        // reduce the argument to [-pi, pi].
        const double    q = _x / two_pi;
        const long long k =
            static_cast<long long>((q >= 0.0) ? (q + 0.5) : (q - 0.5));

        double r = _x - static_cast<double>(k) * two_pi;

        if (r > pi)
        {
            r -= two_pi;
        }

        if (r < -pi)
        {
            r += two_pi;
        }

        // Taylor series: sum_{n>=0} (-1)^n r^(2n) / (2n)!.
        const double r2   = r * r;
        double       term = 1.0;
        double       sum  = 1.0;

        for (int n = 1; n < 16; ++n)
        {
            term = -term * r2 /
                   static_cast<double>((2 * n - 1) * (2 * n));
            sum  = sum + term;
        }

        return sum;
    }

    // sin_c
    //   function: sine, expressed as cos(x - pi/2).
    inline D_CONSTEXPR double
    sin_c(double _x) noexcept
    {
        const double half_pi = 1.57079632679489661923;

        return cos_c(_x - half_pi);
    }

    // all_arithmetic
    //   trait: true when every type in the pack is an arithmetic type. Used to
    // constrain the variadic component constructors so they never shadow copy
    // or array construction.
    template<typename...>
    struct all_arithmetic : std::true_type
    {
    };

    template<typename    _Head,
             typename... _Tail>
    struct all_arithmetic<_Head, _Tail...>
        : std::integral_constant<bool,
              ( std::is_arithmetic<typename std::decay<_Head>::type>::value &&
                all_arithmetic<_Tail...>::value )>
    {
    };

NS_END  // internal


// ===========================================================================
// III. DEFAULT TOLERANCE
// ===========================================================================

// default_tolerance
//   function: the tolerance used by approximate-equality members/free
// functions when the caller does not supply one. A function template (rather
// than a variable template) so it is available without C++14 variable-template
// support and usable directly in default arguments.
template<typename _T = double>
D_CONSTEXPR _T
default_tolerance() noexcept
{
    return static_cast<_T>(1e-12);
}


// ===========================================================================
// IV.  STRUCTURAL TRAITS
// ===========================================================================

// is_vector / is_vector_v / vector_c
//   trait: detects a linalg::vector instantiation.
template<typename _Type>
struct is_vector : std::false_type
{
};

template<typename    _T,
         std::size_t _N>
struct is_vector<vector<_T, _N>> : std::true_type
{
};

// is_matrix / is_matrix_v / matrix_c
//   trait: detects a linalg::matrix instantiation.
template<typename _Type>
struct is_matrix : std::false_type
{
};

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
struct is_matrix<matrix<_T, _Rows, _Cols>> : std::true_type
{
};

// is_square_matrix / is_square_matrix_v / square_matrix_c
//   trait: detects a square matrix (rows == cols).
template<typename _Type>
struct is_square_matrix : std::false_type
{
};

template<typename    _T,
         std::size_t _N>
struct is_square_matrix<matrix<_T, _N, _N>> : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_INLINE_VAR constexpr bool is_vector_v = is_vector<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_matrix_v = is_matrix<_Type>::value;
template<typename _Type>
D_INLINE_VAR constexpr bool is_square_matrix_v = is_square_matrix<_Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
template<typename _Type>
concept vector_c = is_vector<_Type>::value;
template<typename _Type>
concept matrix_c = is_matrix<_Type>::value;
template<typename _Type>
concept square_matrix_c = is_square_matrix<_Type>::value;
#endif

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_COMMON_
