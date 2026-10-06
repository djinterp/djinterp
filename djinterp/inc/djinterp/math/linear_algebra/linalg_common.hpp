/*******************************************************************************
* djinterp [math]                                              linalg_common.hpp
*
* Foundation of the linear-algebra subframework.
*   Single place for the things every linalg header needs: the nested
* djinterp::math::linalg namespace, forward declarations of vector / matrix,
* the road from an element type to the kernels that compute with it, the
* structural trait families (is_vector / is_matrix / is_square_matrix with
* their _v and concept parallels), and the default comparison tolerance.
*
*   A FACE OVER THE C CORE. The kernels are c/math/vec.h's and mat.h's, for
* float, double and long double; internal::linalg_kernel<Type> forwards to
* them through the overloads that name each kernel once for the three, and
* for any other element type -- an integer, a user's numeric type -- runs
* the same operations in the type's own arithmetic, taking a root through
* double as the subframework always has: the generic path, kept by the
* owner's ruling. A root has two forms, the C library's and a pure-
* arithmetic one, correctly rounded both, so the same bit for bit: below
* C++14 a face takes the library's, from C++14 the constant-expression one,
* and from C++20 whichever std::is_constant_evaluated asks for. An angle's
* arc-cosine has no correctly rounded constant form, so an angle is the
* library's at run time at every level, and a constant expression from C++20.
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
*
* path:      /inc/djinterp/math/linear_algebra/linalg_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FORWARD DECLARATIONS
    --------------------
2.  KERNELS
    -------
    1.  Families
    2.  Elementary functions, chosen by level
    3.  The C families
    4.  The generic path
    5.  Scalar helpers, and the headers not yet on the C core
3.  DEFAULT TOLERANCE
    -----------------
4.  STRUCTURAL TRAITS
    -----------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_LINALG_COMMON_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_LINALG_COMMON_HPP 1

// std
#include <cstddef>                       // std::size_t
// djinterp
#include "../../djinterp.hpp"            // framework root
#include "../../c/math/mat.h"            // the C core: d_mat_*
#include "../../c/math/quat.h"           // the C core: d_quat_*
#include "../../c/math/vec.h"            // the C core: d_vec_*, d_math_*
// re_std
#include "../../../re_std/type_traits/decay.hpp"              // decay
#include "../../../re_std/type_traits/integral_constant.hpp"  // integral_
                                                              // constant
#include "../../../re_std/type_traits/is_arithmetic.hpp"      // is_arithmetic
#include "../../../re_std/type_traits/is_same.hpp"            // is_same
// std, from C++20 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <type_traits>               // std::is_constant_evaluated
#endif


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  FORWARD DECLARATIONS
//==============================================================================


// vector
//   class: fixed-size column vector (defined in vector.hpp).
template<typename    Type,
         std::size_t N>
class vector;

// matrix
//   class: fixed-size, row-major matrix (defined in matrix.hpp).
template<typename    Type,
         std::size_t Rows,
         std::size_t Cols>
class matrix;


//==============================================================================
// 2.  KERNELS
//==============================================================================


// 2.1    Families
//------------------------------------------------------------------------------
NS_INTERNAL

    // linalg_family_of
    //   trait: 1 when the C core holds Type (float, double, long double),
    // 0 for the generic path.
    template<typename Type>
    struct linalg_family_of
    {
        enum
        {
            value = ( (re_std::is_same<Type, float>::value)       ||
                      (re_std::is_same<Type, double>::value)      ||
                      (re_std::is_same<Type, long double>::value) ) ? 1 : 0
        };
    };

    // linalg_kernel
    //   trait: the operations on Type's vectors, chosen by its family.
    template<typename Type,
             int      Family = linalg_family_of<Type>::value>
    struct linalg_kernel;

NS_END  // internal

// 2.2    Elementary functions, chosen by level
//------------------------------------------------------------------------------
// The two roots are the same bit for bit; which runs is a matter of what the
// level can evaluate at compile time. The arc-cosine, sine and cosine have no
// correctly rounded constant form, so each is the C library's at run time at
// every level and a constant expression from C++20. Each is one whole
// function per level.
NS_INTERNAL

    template<typename Type>
    struct linalg_elementary
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        static constexpr Type
        sqrt(Type _x) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_math_sqrt_c(_x);
            }

            return d_math_sqrt(_x);
        }
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
        static constexpr Type
        sqrt(Type _x) noexcept
        {
            return d_math_sqrt_c(_x);
        }
#else
        static Type
        sqrt(Type _x) D_NOEXCEPT
        {
            return d_math_sqrt(_x);
        }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

        // acos, sin, cos
        //   the library's at run time; constant expressions from C++20.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        static constexpr Type
        acos(Type _x) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_math_acos_c(_x);
            }

            return d_math_acos(_x);
        }

        static constexpr Type
        sin(Type _x) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_math_sin_c(_x);
            }

            return d_math_sin(_x);
        }

        static constexpr Type
        cos(Type _x) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_math_cos_c(_x);
            }

            return d_math_cos(_x);
        }
#else
        static Type
        acos(Type _x) D_NOEXCEPT
        {
            return d_math_acos(_x);
        }

        static Type
        sin(Type _x) D_NOEXCEPT
        {
            return d_math_sin(_x);
        }

        static Type
        cos(Type _x) D_NOEXCEPT
        {
            return d_math_cos(_x);
        }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
    };

NS_END  // internal

// 2.3    The C families
//------------------------------------------------------------------------------
// One line per operation, forwarding to c/math/vec.h; the roots through
// linalg_elementary, so the C core's _c and plain forms are chosen by level.
NS_INTERNAL

    template<typename Type>
    struct linalg_kernel<Type, 1>
    {
        typedef Type               root_type;
        typedef linalg_elementary<Type> roots;

        static D_CONSTEXPR_CPP14 void
        fill(Type* _out, std::size_t _n, Type _value) D_NOEXCEPT
        {
            d_vec_fill(_out, _n, _value);
        }

        static D_CONSTEXPR_CPP14 void
        add(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
            D_NOEXCEPT
        {
            d_vec_add(_out, _a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 void
        sub(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
            D_NOEXCEPT
        {
            d_vec_sub(_out, _a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 void
        scale(Type* _out, const Type* _a, Type _s, std::size_t _n) D_NOEXCEPT
        {
            d_vec_scale(_out, _a, _s, _n);
        }

        static D_CONSTEXPR_CPP14 void
        divide(Type* _out, const Type* _a, Type _s, std::size_t _n) D_NOEXCEPT
        {
            d_vec_divide(_out, _a, _s, _n);
        }

        static D_CONSTEXPR_CPP14 void
        negate(Type* _out, const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            d_vec_negate(_out, _a, _n);
        }

        static D_CONSTEXPR_CPP14 void
        hadamard(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
            D_NOEXCEPT
        {
            d_vec_hadamard(_out, _a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 void
        lerp(Type*       _out,
             const Type* _a,
             const Type* _b,
             Type        _t,
             std::size_t _n) D_NOEXCEPT
        {
            d_vec_lerp(_out, _a, _b, _t, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        dot(const Type* _a, const Type* _b, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_dot(_a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 void
        cross(Type* _out, const Type* _a, const Type* _b) D_NOEXCEPT
        {
            d_vec_cross(_out, _a, _b);
        }

        static D_CONSTEXPR_CPP14 Type
        sum(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_sum(_a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        product(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_product(_a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        min(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_min(_a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        max(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_max(_a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        norm_squared(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_norm_squared(_a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        distance_squared(const Type* _a, const Type* _b, std::size_t _n)
            D_NOEXCEPT
        {
            return d_vec_distance_squared(_a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        norm(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return roots::sqrt(d_vec_norm_squared(_a, _n));
        }

        static D_CONSTEXPR_CPP14 Type
        distance(const Type* _a, const Type* _b, std::size_t _n) D_NOEXCEPT
        {
            return roots::sqrt(d_vec_distance_squared(_a, _b, _n));
        }

        static D_CONSTEXPR_CPP14 void
        project(Type*       _out,
                const Type* _a,
                const Type* _onto,
                std::size_t _n) D_NOEXCEPT
        {
            d_vec_project(_out, _a, _onto, _n);
        }

        static D_CONSTEXPR_CPP14 void
        reject(Type*       _out,
               const Type* _a,
               const Type* _from,
               std::size_t _n) D_NOEXCEPT
        {
            d_vec_reject(_out, _a, _from, _n);
        }

        static D_CONSTEXPR_CPP14 bool
        equal(const Type* _a, const Type* _b, std::size_t _n) D_NOEXCEPT
        {
            return d_vec_equal(_a, _b, _n);
        }

        static D_CONSTEXPR_CPP14 bool
        equals(const Type* _a,
               const Type* _b,
               Type        _tolerance,
               std::size_t _n) D_NOEXCEPT
        {
            return d_vec_equals(_a, _b, _tolerance, _n);
        }

        // ---- matrices: row-major arrays, the shape given --------------------

        static D_CONSTEXPR_CPP14 void
        identity(Type* _out, std::size_t _n) D_NOEXCEPT
        {
            d_mat_identity(_out, _n);
        }

        static D_CONSTEXPR_CPP14 void
        diagonal(Type* _out, const Type* _d, std::size_t _n) D_NOEXCEPT
        {
            d_mat_diagonal(_out, _d, _n);
        }

        static D_CONSTEXPR_CPP14 void
        row(Type*       _out,
            const Type* _a,
            std::size_t _i,
            std::size_t _cols) D_NOEXCEPT
        {
            d_mat_row(_out, _a, _i, _cols);
        }

        static D_CONSTEXPR_CPP14 void
        col(Type*       _out,
            const Type* _a,
            std::size_t _j,
            std::size_t _rows,
            std::size_t _cols) D_NOEXCEPT
        {
            d_mat_col(_out, _a, _j, _rows, _cols);
        }

        static D_CONSTEXPR_CPP14 void
        transpose(Type*       _out,
                  const Type* _a,
                  std::size_t _rows,
                  std::size_t _cols) D_NOEXCEPT
        {
            d_mat_transpose(_out, _a, _rows, _cols);
        }

        static D_CONSTEXPR_CPP14 void
        multiply(Type*       _out,
                 const Type* _a,
                 const Type* _b,
                 std::size_t _rows,
                 std::size_t _inner,
                 std::size_t _cols) D_NOEXCEPT
        {
            d_mat_multiply(_out, _a, _b, _rows, _inner, _cols);
        }

        static D_CONSTEXPR_CPP14 void
        multiply_vector(Type*       _out,
                        const Type* _a,
                        const Type* _v,
                        std::size_t _rows,
                        std::size_t _cols) D_NOEXCEPT
        {
            d_mat_multiply_vector(_out, _a, _v, _rows, _cols);
        }

        static D_CONSTEXPR_CPP14 void
        power(Type*       _out,
              Type*       _scratch,
              const Type* _a,
              std::size_t _n,
              std::size_t _exponent) D_NOEXCEPT
        {
            d_mat_power(_out, _scratch, _a, _n, _exponent);
        }

        static D_CONSTEXPR_CPP14 Type
        trace(const Type* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_mat_trace(_a, _n);
        }

        static D_CONSTEXPR_CPP14 bool
        is_symmetric(const Type* _a, std::size_t _n, Type _tolerance)
            D_NOEXCEPT
        {
            return d_mat_is_symmetric(_a, _n, _tolerance);
        }

        static D_CONSTEXPR_CPP14 bool
        is_identity(const Type* _a, std::size_t _n, Type _tolerance)
            D_NOEXCEPT
        {
            return d_mat_is_identity(_a, _n, _tolerance);
        }

        static D_CONSTEXPR_CPP14 Type
        determinant(const Type* _a, Type* _scratch, std::size_t _n)
            D_NOEXCEPT
        {
            return d_mat_determinant(_a, _scratch, _n);
        }

        static D_CONSTEXPR_CPP14 bool
        inverse(Type*       _out,
                Type*       _scratch,
                const Type* _a,
                std::size_t _n) D_NOEXCEPT
        {
            return d_mat_inverse(_out, _scratch, _a, _n);
        }

        static D_CONSTEXPR_CPP14 void
        submatrix(Type*       _out,
                  const Type* _a,
                  std::size_t _n,
                  std::size_t _row,
                  std::size_t _col) D_NOEXCEPT
        {
            d_mat_submatrix(_out, _a, _n, _row, _col);
        }

        static D_CONSTEXPR_CPP14 Type
        cofactor(const Type* _a,
                 Type*       _scratch,
                 std::size_t _n,
                 std::size_t _row,
                 std::size_t _col) D_NOEXCEPT
        {
            return d_mat_cofactor(_a, _scratch, _n, _row, _col);
        }

        static D_CONSTEXPR_CPP14 void
        cofactor_matrix(Type*       _out,
                        const Type* _a,
                        Type*       _scratch,
                        std::size_t _n,
                        bool        _adjugate) D_NOEXCEPT
        {
            d_mat_cofactor_matrix(_out, _a, _scratch, _n, _adjugate);
        }

        static D_CONSTEXPR_CPP14 void
        rotation_2d(Type* _out, Type _c, Type _s) D_NOEXCEPT
        {
            d_mat_rotation_2d(_out, _c, _s);
        }

        static D_CONSTEXPR_CPP14 void
        rotation_3d(Type* _out, std::size_t _axis, Type _c, Type _s)
            D_NOEXCEPT
        {
            d_mat_rotation_3d(_out, _axis, _c, _s);
        }

        static D_CONSTEXPR_CPP14 void
        rotation_axis(Type* _out, const Type* _k, Type _c, Type _s) D_NOEXCEPT
        {
            d_mat_rotation_axis(_out, _k, _c, _s);
        }

        // ---- quaternions: arrays [x, y, z, w] ------------------------------

        static D_CONSTEXPR_CPP14 void
        quat_multiply(Type* _out, const Type* _a, const Type* _b) D_NOEXCEPT
        {
            d_quat_multiply(_out, _a, _b);
        }

        static D_CONSTEXPR_CPP14 void
        quat_conjugate(Type* _out, const Type* _q) D_NOEXCEPT
        {
            d_quat_conjugate(_out, _q);
        }

        static D_CONSTEXPR_CPP14 void
        quat_normalize(Type* _out, const Type* _q) D_NOEXCEPT
        {
            d_quat_normalize(_out, _q, norm(_q, 4));
        }

        static D_CONSTEXPR_CPP14 void
        quat_rotate(Type* _out, const Type* _q, const Type* _v) D_NOEXCEPT
        {
            d_quat_rotate(_out, _q, _v);
        }

        static D_CONSTEXPR_CPP14 void
        quat_to_matrix3(Type* _out, const Type* _q) D_NOEXCEPT
        {
            d_quat_to_matrix3(_out, _q);
        }

        static D_CONSTEXPR_CPP14 void
        quat_to_matrix4(Type* _out, const Type* _q) D_NOEXCEPT
        {
            d_quat_to_matrix4(_out, _q);
        }

        // the three that need a sine or a cosine: the C library's at run
        // time, constant expressions from C++20
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        static constexpr void
        quat_from_axis_angle(Type*       _out,
                             const Type* _axis,
                             Type        _angle) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_quat_from_axis_angle_c(_out, _axis, _angle);
            }
            else
            {
                d_quat_from_axis_angle(_out, _axis, _angle);
            }
        }

        static constexpr void
        quat_from_unit_axis_angle(Type*       _out,
                                  const Type* _axis,
                                  Type        _angle) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_quat_from_unit_axis_angle_c(_out, _axis, _angle);
            }
            else
            {
                d_quat_from_unit_axis_angle(_out, _axis, _angle);
            }
        }

        static constexpr void
        quat_slerp(Type*       _out,
                   const Type* _a,
                   const Type* _b,
                   Type        _t) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_quat_slerp_c(_out, _a, _b, _t);
            }
            else
            {
                d_quat_slerp(_out, _a, _b, _t);
            }
        }
#else
        static void
        quat_from_axis_angle(Type*       _out,
                             const Type* _axis,
                             Type        _angle) D_NOEXCEPT
        {
            d_quat_from_axis_angle(_out, _axis, _angle);
        }

        static void
        quat_from_unit_axis_angle(Type*       _out,
                                  const Type* _axis,
                                  Type        _angle) D_NOEXCEPT
        {
            d_quat_from_unit_axis_angle(_out, _axis, _angle);
        }

        static void
        quat_slerp(Type*       _out,
                   const Type* _a,
                   const Type* _b,
                   Type        _t) D_NOEXCEPT
        {
            d_quat_slerp(_out, _a, _b, _t);
        }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
    };

NS_END  // internal

// 2.4    The generic path
//------------------------------------------------------------------------------
// The same operations in Type's own arithmetic, in the subframework's former
// expressions; a root is taken through double, as it always was.
NS_INTERNAL

    template<typename Type>
    struct linalg_kernel<Type, 0>
    {
        typedef double               root_type;
        typedef linalg_elementary<double> roots;

        static D_CONSTEXPR_CPP14 void
        fill(Type* _out, std::size_t _n, Type _value)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _value;
            }
        }

        static D_CONSTEXPR_CPP14 void
        add(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] + _b[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        sub(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] - _b[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        scale(Type* _out, const Type* _a, Type _s, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] * _s;
            }
        }

        static D_CONSTEXPR_CPP14 void
        divide(Type* _out, const Type* _a, Type _s, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] / _s;
            }
        }

        static D_CONSTEXPR_CPP14 void
        negate(Type* _out, const Type* _a, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = -_a[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        hadamard(Type* _out, const Type* _a, const Type* _b, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] * _b[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        lerp(Type*       _out,
             const Type* _a,
             const Type* _b,
             Type        _t,
             std::size_t _n)
        {
            const Type keep = static_cast<Type>(1) - _t;

            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = (_a[i] * keep) + (_b[i] * _t);
            }
        }

        static D_CONSTEXPR_CPP14 Type
        dot(const Type* _a, const Type* _b, std::size_t _n)
        {
            Type acc = static_cast<Type>(0);

            for (std::size_t i = 0; i < _n; ++i)
            {
                acc += _a[i] * _b[i];
            }

            return acc;
        }

        static D_CONSTEXPR_CPP14 void
        cross(Type* _out, const Type* _a, const Type* _b)
        {
            const Type x = (_a[1] * _b[2]) - (_a[2] * _b[1]);
            const Type y = (_a[2] * _b[0]) - (_a[0] * _b[2]);
            const Type z = (_a[0] * _b[1]) - (_a[1] * _b[0]);

            _out[0] = x;
            _out[1] = y;
            _out[2] = z;
        }

        static D_CONSTEXPR_CPP14 Type
        sum(const Type* _a, std::size_t _n)
        {
            Type acc = static_cast<Type>(0);

            for (std::size_t i = 0; i < _n; ++i)
            {
                acc += _a[i];
            }

            return acc;
        }

        static D_CONSTEXPR_CPP14 Type
        product(const Type* _a, std::size_t _n)
        {
            Type acc = static_cast<Type>(1);

            for (std::size_t i = 0; i < _n; ++i)
            {
                acc *= _a[i];
            }

            return acc;
        }

        static D_CONSTEXPR_CPP14 Type
        min(const Type* _a, std::size_t _n)
        {
            Type m = _a[0];

            for (std::size_t i = 1; i < _n; ++i)
            {
                if (_a[i] < m)
                {
                    m = _a[i];
                }
            }

            return m;
        }

        static D_CONSTEXPR_CPP14 Type
        max(const Type* _a, std::size_t _n)
        {
            Type m = _a[0];

            for (std::size_t i = 1; i < _n; ++i)
            {
                if (_a[i] > m)
                {
                    m = _a[i];
                }
            }

            return m;
        }

        static D_CONSTEXPR_CPP14 Type
        norm_squared(const Type* _a, std::size_t _n)
        {
            return dot(_a, _a, _n);
        }

        static D_CONSTEXPR_CPP14 Type
        distance_squared(const Type* _a, const Type* _b, std::size_t _n)
        {
            Type acc = static_cast<Type>(0);

            for (std::size_t i = 0; i < _n; ++i)
            {
                const Type d = _a[i] - _b[i];

                acc += d * d;
            }

            return acc;
        }

        static D_CONSTEXPR_CPP14 Type
        norm(const Type* _a, std::size_t _n)
        {
            return static_cast<Type>(
                roots::sqrt(static_cast<double>(norm_squared(_a, _n))));
        }

        static D_CONSTEXPR_CPP14 Type
        distance(const Type* _a, const Type* _b, std::size_t _n)
        {
            return static_cast<Type>(roots::sqrt(
                static_cast<double>(distance_squared(_a, _b, _n))));
        }

        static D_CONSTEXPR_CPP14 void
        project(Type*       _out,
                const Type* _a,
                const Type* _onto,
                std::size_t _n)
        {
            const Type d = norm_squared(_onto, _n);

            // onto the zero vector there is nothing to project
            if (d > static_cast<Type>(0))
            {
                scale(_out, _onto, dot(_a, _onto, _n) / d, _n);
            }
            else
            {
                fill(_out, _n, static_cast<Type>(0));
            }
        }

        static D_CONSTEXPR_CPP14 void
        reject(Type*       _out,
               const Type* _a,
               const Type* _from,
               std::size_t _n)
        {
            const Type d     = norm_squared(_from, _n);
            const Type ratio = (d > static_cast<Type>(0))
                                   ? (dot(_a, _from, _n) / d)
                                   : static_cast<Type>(0);

            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[i] = _a[i] - (_from[i] * ratio);
            }
        }

        static D_CONSTEXPR_CPP14 bool
        equal(const Type* _a, const Type* _b, std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                if (!(_a[i] == _b[i]))
                {
                    return false;
                }
            }

            return true;
        }

        static D_CONSTEXPR_CPP14 bool
        equals(const Type* _a,
               const Type* _b,
               Type        _tolerance,
               std::size_t _n)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                const Type d = _a[i] - _b[i];

                if (((d < static_cast<Type>(0)) ? -d : d) > _tolerance)
                {
                    return false;
                }
            }

            return true;
        }

        // ---- matrices: row-major arrays, the shape given --------------------

        static D_CONSTEXPR_CPP14 void
        identity(Type* _out, std::size_t _n)
        {
            fill(_out, _n * _n, static_cast<Type>(0));

            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[(i * _n) + i] = static_cast<Type>(1);
            }
        }

        static D_CONSTEXPR_CPP14 void
        diagonal(Type* _out, const Type* _d, std::size_t _n)
        {
            fill(_out, _n * _n, static_cast<Type>(0));

            for (std::size_t i = 0; i < _n; ++i)
            {
                _out[(i * _n) + i] = _d[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        row(Type*       _out,
            const Type* _a,
            std::size_t _i,
            std::size_t _cols)
        {
            for (std::size_t j = 0; j < _cols; ++j)
            {
                _out[j] = _a[(_i * _cols) + j];
            }
        }

        static D_CONSTEXPR_CPP14 void
        col(Type*       _out,
            const Type* _a,
            std::size_t _j,
            std::size_t _rows,
            std::size_t _cols)
        {
            for (std::size_t i = 0; i < _rows; ++i)
            {
                _out[i] = _a[(i * _cols) + _j];
            }
        }

        static D_CONSTEXPR_CPP14 void
        transpose(Type*       _out,
                  const Type* _a,
                  std::size_t _rows,
                  std::size_t _cols)
        {
            for (std::size_t i = 0; i < _rows; ++i)
            {
                for (std::size_t j = 0; j < _cols; ++j)
                {
                    _out[(j * _rows) + i] = _a[(i * _cols) + j];
                }
            }
        }

        static D_CONSTEXPR_CPP14 void
        multiply(Type*       _out,
                 const Type* _a,
                 const Type* _b,
                 std::size_t _rows,
                 std::size_t _inner,
                 std::size_t _cols)
        {
            for (std::size_t i = 0; i < _rows; ++i)
            {
                for (std::size_t j = 0; j < _cols; ++j)
                {
                    Type acc = static_cast<Type>(0);

                    for (std::size_t k = 0; k < _inner; ++k)
                    {
                        acc += _a[(i * _inner) + k] * _b[(k * _cols) + j];
                    }

                    _out[(i * _cols) + j] = acc;
                }
            }
        }

        static D_CONSTEXPR_CPP14 void
        multiply_vector(Type*       _out,
                        const Type* _a,
                        const Type* _v,
                        std::size_t _rows,
                        std::size_t _cols)
        {
            for (std::size_t i = 0; i < _rows; ++i)
            {
                _out[i] = dot(_a + (i * _cols), _v, _cols);
            }
        }

        static D_CONSTEXPR_CPP14 void
        power(Type*       _out,
              Type*       _scratch,
              const Type* _a,
              std::size_t _n,
              std::size_t _exponent)
        {
            identity(_out, _n);

            for (std::size_t k = 0; k < _exponent; ++k)
            {
                multiply(_scratch, _a, _out, _n, _n, _n);

                for (std::size_t i = 0; i < (_n * _n); ++i)
                {
                    _out[i] = _scratch[i];
                }
            }
        }

        static D_CONSTEXPR_CPP14 Type
        trace(const Type* _a, std::size_t _n)
        {
            Type acc = static_cast<Type>(0);

            for (std::size_t i = 0; i < _n; ++i)
            {
                acc += _a[(i * _n) + i];
            }

            return acc;
        }

        static D_CONSTEXPR_CPP14 bool
        is_symmetric(const Type* _a, std::size_t _n, Type _tolerance)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                for (std::size_t j = i + 1; j < _n; ++j)
                {
                    const Type d = _a[(i * _n) + j] - _a[(j * _n) + i];

                    if (((d < static_cast<Type>(0)) ? -d : d) > _tolerance)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        static D_CONSTEXPR_CPP14 bool
        is_identity(const Type* _a, std::size_t _n, Type _tolerance)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                for (std::size_t j = 0; j < _n; ++j)
                {
                    const Type d =
                        _a[(i * _n) + j] - ((i == j) ? static_cast<Type>(1)
                                                     : static_cast<Type>(0));

                    if (((d < static_cast<Type>(0)) ? -d : d) > _tolerance)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        // ---- the square algebra, in the subframework's former expressions -

        static D_CONSTEXPR_CPP14 Type
        determinant(const Type* _a, Type* _scratch, std::size_t _n)
        {
            Type prev = static_cast<Type>(1);
            Type sign = static_cast<Type>(1);

            for (std::size_t k = 0; k < (_n * _n); ++k)
            {
                _scratch[k] = _a[k];
            }

            for (std::size_t k = 0; k < _n; ++k)
            {
                // a zero pivot: swap in the first row below with a non-zero
                if (_scratch[(k * _n) + k] == static_cast<Type>(0))
                {
                    std::size_t swap_row = k;

                    for (std::size_t r = k + 1; r < _n; ++r)
                    {
                        if (!(_scratch[(r * _n) + k] == static_cast<Type>(0)))
                        {
                            swap_row = r;
                            break;
                        }
                    }

                    if (swap_row == k)
                    {
                        return static_cast<Type>(0);
                    }

                    for (std::size_t c = 0; c < _n; ++c)
                    {
                        const Type t = _scratch[(k * _n) + c];

                        _scratch[(k * _n) + c]        =
                            _scratch[(swap_row * _n) + c];
                        _scratch[(swap_row * _n) + c] = t;
                    }

                    sign = -sign;
                }

                for (std::size_t i = k + 1; i < _n; ++i)
                {
                    for (std::size_t j = k + 1; j < _n; ++j)
                    {
                        _scratch[(i * _n) + j] =
                            ( (_scratch[(k * _n) + k] *
                               _scratch[(i * _n) + j]) -
                              (_scratch[(i * _n) + k] *
                               _scratch[(k * _n) + j]) ) / prev;
                    }
                }

                prev = _scratch[(k * _n) + k];
            }

            return sign * prev;
        }

        static D_CONSTEXPR_CPP14 bool
        inverse(Type*       _out,
                Type*       _scratch,
                const Type* _a,
                std::size_t _n)
        {
            for (std::size_t k = 0; k < (_n * _n); ++k)
            {
                _scratch[k] = _a[k];
            }

            identity(_out, _n);

            for (std::size_t col = 0; col < _n; ++col)
            {
                std::size_t pivot = col;
                Type        maxv  = abs_of(_scratch[(col * _n) + col]);

                for (std::size_t r = col + 1; r < _n; ++r)
                {
                    const Type v = abs_of(_scratch[(r * _n) + col]);

                    if (v > maxv)
                    {
                        maxv  = v;
                        pivot = r;
                    }
                }

                // no pivot: singular
                if (_scratch[(pivot * _n) + col] == static_cast<Type>(0))
                {
                    fill(_out, _n * _n, static_cast<Type>(0));

                    return false;
                }

                if (pivot != col)
                {
                    for (std::size_t c = 0; c < _n; ++c)
                    {
                        const Type ta = _scratch[(col * _n) + c];
                        const Type ti = _out[(col * _n) + c];

                        _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                        _scratch[(pivot * _n) + c] = ta;
                        _out[(col * _n) + c]       = _out[(pivot * _n) + c];
                        _out[(pivot * _n) + c]     = ti;
                    }
                }

                const Type d = _scratch[(col * _n) + col];

                for (std::size_t c = 0; c < _n; ++c)
                {
                    _scratch[(col * _n) + c] = _scratch[(col * _n) + c] / d;
                    _out[(col * _n) + c]     = _out[(col * _n) + c] / d;
                }

                for (std::size_t r = 0; r < _n; ++r)
                {
                    if (r != col)
                    {
                        const Type f = _scratch[(r * _n) + col];

                        for (std::size_t c = 0; c < _n; ++c)
                        {
                            _scratch[(r * _n) + c] =
                                _scratch[(r * _n) + c] -
                                (f * _scratch[(col * _n) + c]);
                            _out[(r * _n) + c] =
                                _out[(r * _n) + c] - (f * _out[(col * _n) + c]);
                        }
                    }
                }
            }

            return true;
        }

        static D_CONSTEXPR_CPP14 void
        submatrix(Type*       _out,
                  const Type* _a,
                  std::size_t _n,
                  std::size_t _row,
                  std::size_t _col)
        {
            std::size_t rr = 0;

            for (std::size_t r = 0; r < _n; ++r)
            {
                if (r != _row)
                {
                    std::size_t cc = 0;

                    for (std::size_t c = 0; c < _n; ++c)
                    {
                        if (c != _col)
                        {
                            _out[(rr * (_n - 1)) + cc] = _a[(r * _n) + c];
                            ++cc;
                        }
                    }

                    ++rr;
                }
            }
        }

        static D_CONSTEXPR_CPP14 Type
        cofactor(const Type* _a,
                 Type*       _scratch,
                 std::size_t _n,
                 std::size_t _row,
                 std::size_t _col)
        {
            const std::size_t m    = _n - 1;
            const Type        sign = (((_row + _col) % 2) == 0)
                                         ? static_cast<Type>(1)
                                         : static_cast<Type>(-1);

            submatrix(_scratch, _a, _n, _row, _col);

            return sign * determinant(_scratch, _scratch + (m * m), m);
        }

        static D_CONSTEXPR_CPP14 void
        cofactor_matrix(Type*       _out,
                        const Type* _a,
                        Type*       _scratch,
                        std::size_t _n,
                        bool        _adjugate)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                for (std::size_t j = 0; j < _n; ++j)
                {
                    const Type c = cofactor(_a, _scratch, _n, i, j);

                    _out[_adjugate ? ((j * _n) + i) : ((i * _n) + j)] = c;
                }
            }
        }

        // ---- rotations, in transform.hpp's former expressions --------------

        static D_CONSTEXPR_CPP14 void
        rotation_2d(Type* _out, Type _c, Type _s)
        {
            _out[0] = _c;
            _out[1] = -_s;
            _out[2] = _s;
            _out[3] = _c;
        }

        static D_CONSTEXPR_CPP14 void
        rotation_3d(Type* _out, std::size_t _axis, Type _c, Type _s)
        {
            const std::size_t a = (_axis + 1) % 3;
            const std::size_t b = (_axis + 2) % 3;

            identity(_out, 3);

            _out[(a * 3) + a] = _c;
            _out[(a * 3) + b] = -_s;
            _out[(b * 3) + a] = _s;
            _out[(b * 3) + b] = _c;
        }

        static D_CONSTEXPR_CPP14 void
        rotation_axis(Type* _out, const Type* _k, Type _c, Type _s)
        {
            const Type t  = static_cast<Type>(1) - _c;
            const Type kx = _k[0];
            const Type ky = _k[1];
            const Type kz = _k[2];

            _out[0] = _c + kx * kx * t;
            _out[1] = kx * ky * t - kz * _s;
            _out[2] = kx * kz * t + ky * _s;
            _out[3] = ky * kx * t + kz * _s;
            _out[4] = _c + ky * ky * t;
            _out[5] = ky * kz * t - kx * _s;
            _out[6] = kz * kx * t - ky * _s;
            _out[7] = kz * ky * t + kx * _s;
            _out[8] = _c + kz * kz * t;
        }

        // ---- quaternions, in the quaternion headers' former expressions ----

        static D_CONSTEXPR_CPP14 void
        quat_multiply(Type* _out, const Type* _a, const Type* _b)
        {
            const Type x = _a[3] * _b[0] + _a[0] * _b[3] + _a[1] * _b[2] -
                           _a[2] * _b[1];
            const Type y = _a[3] * _b[1] - _a[0] * _b[2] + _a[1] * _b[3] +
                           _a[2] * _b[0];
            const Type z = _a[3] * _b[2] + _a[0] * _b[1] - _a[1] * _b[0] +
                           _a[2] * _b[3];
            const Type w = _a[3] * _b[3] - _a[0] * _b[0] - _a[1] * _b[1] -
                           _a[2] * _b[2];

            _out[0] = x;
            _out[1] = y;
            _out[2] = z;
            _out[3] = w;
        }

        static D_CONSTEXPR_CPP14 void
        quat_conjugate(Type* _out, const Type* _q)
        {
            _out[0] = -_q[0];
            _out[1] = -_q[1];
            _out[2] = -_q[2];
            _out[3] = _q[3];
        }

        static D_CONSTEXPR_CPP14 void
        quat_normalize(Type* _out, const Type* _q)
        {
            const Type n = norm(_q, 4);

            for (std::size_t i = 0; i < 4; ++i)
            {
                _out[i] = (n == static_cast<Type>(0)) ? _q[i] : (_q[i] / n);
            }
        }

        static D_CONSTEXPR_CPP14 void
        quat_rotate(Type* _out, const Type* _q, const Type* _v)
        {
            Type t[3]  = { Type(), Type(), Type() };
            Type ut[3] = { Type(), Type(), Type() };

            cross(t, _q, _v);
            scale(t, t, static_cast<Type>(2), 3);
            cross(ut, _q, t);

            for (std::size_t i = 0; i < 3; ++i)
            {
                _out[i] = _v[i] + (t[i] * _q[3]) + ut[i];
            }
        }

        static D_CONSTEXPR_CPP14 void
        quat_to_matrix3(Type* _out, const Type* _q)
        {
            const Type x   = _q[0];
            const Type y   = _q[1];
            const Type z   = _q[2];
            const Type w   = _q[3];
            const Type one = static_cast<Type>(1);
            const Type two = static_cast<Type>(2);

            _out[0] = one - (two * ((y * y) + (z * z)));
            _out[1] = two * ((x * y) - (w * z));
            _out[2] = two * ((x * z) + (w * y));
            _out[3] = two * ((x * y) + (w * z));
            _out[4] = one - (two * ((x * x) + (z * z)));
            _out[5] = two * ((y * z) - (w * x));
            _out[6] = two * ((x * z) - (w * y));
            _out[7] = two * ((y * z) + (w * x));
            _out[8] = one - (two * ((x * x) + (y * y)));
        }

        static D_CONSTEXPR_CPP14 void
        quat_to_matrix4(Type* _out, const Type* _q)
        {
            Type r[9] = { Type(), Type(), Type(), Type(), Type(),
                          Type(), Type(), Type(), Type() };

            quat_to_matrix3(r, _q);
            identity(_out, 4);

            for (std::size_t i = 0; i < 3; ++i)
            {
                for (std::size_t j = 0; j < 3; ++j)
                {
                    _out[(i * 4) + j] = r[(i * 3) + j];
                }
            }
        }

        // the sine and cosine through double, as the angles are
        static D_CONSTEXPR_CPP20 void
        quat_from_axis_angle(Type* _out, const Type* _axis, Type _angle)
        {
            const Type length = norm(_axis, 3);

            // a zero axis names no rotation
            if (length == static_cast<Type>(0))
            {
                _out[0] = static_cast<Type>(0);
                _out[1] = static_cast<Type>(0);
                _out[2] = static_cast<Type>(0);
                _out[3] = static_cast<Type>(1);

                return;
            }

            const Type half = _angle / static_cast<Type>(2);
            const Type s    = static_cast<Type>(roots::sin(
                                  static_cast<double>(half))) / length;

            _out[0] = _axis[0] * s;
            _out[1] = _axis[1] * s;
            _out[2] = _axis[2] * s;
            _out[3] = static_cast<Type>(roots::cos(static_cast<double>(half)));
        }

        static D_CONSTEXPR_CPP20 void
        quat_from_unit_axis_angle(Type*       _out,
                                  const Type* _axis,
                                  Type        _angle)
        {
            const Type half = _angle / static_cast<Type>(2);
            const Type s    = static_cast<Type>(roots::sin(
                                  static_cast<double>(half)));

            _out[0] = _axis[0] * s;
            _out[1] = _axis[1] * s;
            _out[2] = _axis[2] * s;
            _out[3] = static_cast<Type>(roots::cos(static_cast<double>(half)));
        }

        static D_CONSTEXPR_CPP20 void
        quat_slerp(Type* _out, const Type* _a, const Type* _b, Type _t)
        {
            Type b[4] = { _b[0], _b[1], _b[2], _b[3] };
            Type d    = dot(_a, _b, 4);

            // q and -q are the same rotation: take the shorter arc
            if (d < static_cast<Type>(0))
            {
                negate(b, b, 4);
                d = -d;
            }

            // nearly parallel: the arc is a line, renormalized
            if (d > static_cast<Type>(0.9995))
            {
                Type l[4] = { Type(), Type(), Type(), Type() };

                for (std::size_t i = 0; i < 4; ++i)
                {
                    l[i] = _a[i] + (_t * (b[i] - _a[i]));
                }

                quat_normalize(_out, l);

                return;
            }

            const Type theta_0 = static_cast<Type>(roots::acos(
                                     static_cast<double>(d)));
            const Type theta   = theta_0 * _t;
            const Type sin_0   = static_cast<Type>(roots::sin(
                                     static_cast<double>(theta_0)));
            const Type s_a     = static_cast<Type>(roots::sin(
                                     static_cast<double>(theta_0 - theta))) /
                                 sin_0;
            const Type s_b     = static_cast<Type>(roots::sin(
                                     static_cast<double>(theta))) / sin_0;

            for (std::size_t i = 0; i < 4; ++i)
            {
                _out[i] = (s_a * _a[i]) + (s_b * b[i]);
            }
        }

    private:
        static D_CONSTEXPR_CPP14 Type
        abs_of(Type _x)
        {
            return (_x < static_cast<Type>(0)) ? -_x : _x;
        }
    };

NS_END  // internal

// 2.5    Scalar helpers, and the headers not yet on the C core
//------------------------------------------------------------------------------
// transform, decomposition, eigen, svd and dynamic_matrix still call these.
// sqrt_c and acos_c are the C core's now -- correctly rounded, and within an
// ulp -- rather than a hundred Newton steps and a
// 6.7e-5 polynomial; cos_c and sin_c keep their series, reduced without
// `long long`, which ISO strict C++98 has not. They go when those headers
// move to the C core.
NS_INTERNAL

    // abs_c
    //   function: absolute value of a scalar.
    template<typename Type>
    inline D_CONSTEXPR Type
    abs_c(Type _x) D_NOEXCEPT
    {
        return (_x < static_cast<Type>(0)) ? -_x : _x;
    }

    // sqrt_c
    //   function: the correctly rounded square root, as a constant expression
    // from C++14 (non-positive input -> 0, as before).
    inline D_CONSTEXPR_CPP14 double
    sqrt_c(double _x) D_NOEXCEPT
    {
        return (_x <= 0.0) ? 0.0 : d_math_d_sqrt_c(_x);
    }

    // acos_c
    //   function: arc-cosine over [-1, 1], in radians, within an ulp; an
    // input outside the domain is clamped into it, as before.
    inline D_CONSTEXPR_CPP14 double
    acos_c(double _x) D_NOEXCEPT
    {
        return d_math_d_acos_c( (_x < -1.0) ? -1.0 :
                                (_x >  1.0) ?  1.0 : _x );
    }

    // cos_c
    //   function: cosine, via range reduction to [-pi, pi] and a Taylor
    // series. Accurate to roughly 1e-13 across the reduced range.
    inline D_CONSTEXPR_CPP14 double
    cos_c(double _x) D_NOEXCEPT
    {
        const double pi     = 3.14159265358979323846;
        const double two_pi = 6.28318530717958647692;
        // the nearest whole number of turns: 1.5 * 2^52 rounds a double of
        // smaller magnitude to an integer, in pure arithmetic
        const double turns  = _x / two_pi;
        const double magic  = 6755399441055744.0;
        const double k      = (turns < 4503599627370496.0 &&
                               turns > -4503599627370496.0)
                                  ? ((turns + magic) - magic)
                                  : 0.0;
        double       r      = _x - (k * two_pi);

        // into [-pi, pi]
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
                   static_cast<double>(((2 * n) - 1) * (2 * n));
            sum  = sum + term;
        }

        return sum;
    }

    // sin_c
    //   function: sine, expressed as cos(x - pi/2).
    inline D_CONSTEXPR_CPP14 double
    sin_c(double _x) D_NOEXCEPT
    {
        const double half_pi = 1.57079632679489661923;

        return cos_c(_x - half_pi);
    }

    // linalg_requires
    //   trait: complete only when Condition holds, so check() names a
    // member of an incomplete type, and fails to compile, otherwise: a
    // compile-time assertion inside a member function that, unlike the root's
    // C++98 D_STATIC_ASSERT, declares no local typedef for -Wall to call
    // unused. (A member such as z() asserts its dimension only when used.)
    template<bool Condition>
    struct linalg_requires
    {
        static D_CONSTEXPR bool
        check() D_NOEXCEPT
        {
            return true;
        }
    };

    template<>
    struct linalg_requires<false>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // all_arithmetic
    //   trait: true when every type in the pack is an arithmetic type. Used to
    // constrain the variadic component constructors (C++11) so they never
    // shadow copy or array construction.
    template<typename...>
    struct all_arithmetic : re_std::integral_constant<bool, true>
    {};

    template<typename    Head,
             typename... Tail>
    struct all_arithmetic<Head, Tail...>
        : re_std::integral_constant<bool,
              ( re_std::is_arithmetic<
                    typename re_std::decay<Head>::type>::value &&
                all_arithmetic<Tail...>::value )>
    {};
#endif

NS_END  // internal


//==============================================================================
// 3.  DEFAULT TOLERANCE
//==============================================================================


// default_tolerance
//   function: the tolerance approximate-equality members and free functions
// use when the caller supplies none, 1e-12: as a double, or as
// default_tolerance<Type>() in another type. (The default template argument
// it once had is C++11's; the double overload serves every level.)
inline D_CONSTEXPR double
default_tolerance() D_NOEXCEPT
{
    return 1e-12;
}

template<typename Type>
inline D_CONSTEXPR Type
default_tolerance() D_NOEXCEPT
{
    return static_cast<Type>(1e-12);
}


//==============================================================================
// 4.  STRUCTURAL TRAITS
//==============================================================================


// is_vector
//   trait: detects a linalg::vector instantiation.
template<typename Type>
struct is_vector : re_std::integral_constant<bool, false>
{};

template<typename    Type,
         std::size_t N>
struct is_vector<vector<Type, N> > : re_std::integral_constant<bool, true>
{};

// is_matrix
//   trait: detects a linalg::matrix instantiation.
template<typename Type>
struct is_matrix : re_std::integral_constant<bool, false>
{};

template<typename    Type,
         std::size_t Rows,
         std::size_t Cols>
struct is_matrix<matrix<Type, Rows, Cols> >
    : re_std::integral_constant<bool, true>
{};

// is_square_matrix
//   trait: detects a square matrix (rows == cols).
template<typename Type>
struct is_square_matrix : re_std::integral_constant<bool, false>
{};

template<typename    Type,
         std::size_t N>
struct is_square_matrix<matrix<Type, N, N> >
    : re_std::integral_constant<bool, true>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_vector_v / is_matrix_v / is_square_matrix_v
//   variable template: the traits' values (C++14).
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_vector_v = is_vector<Type>::value;

template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_matrix_v = is_matrix<Type>::value;

template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_square_matrix_v =
    is_square_matrix<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// vector_c / matrix_c / square_matrix_c
//   concept: the traits as constraints (C++20).
template<typename Type>
concept vector_c = is_vector<Type>::value;

template<typename Type>
concept matrix_c = is_matrix<Type>::value;

template<typename Type>
concept square_matrix_c = is_square_matrix<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_LINALG_COMMON_HPP
