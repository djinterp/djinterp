/*******************************************************************************
* djinterp [math]                                                     vector.hpp
*
* Fixed-size column vector for the linear-algebra subframework.
*   vector<Type, N> stores its components by value in an array, and every
* operation returns a new vector rather than mutating in place, which is
* what lets them chain. A face over the C core, c/math/vec.h: for float,
* double and long double each operation is one of its kernels over the
* vector's own array, which the constant dimension unrolls; for any other
* element type, the generic path (linalg_common.hpp).
*
* TWO SPELLINGS (see linalg_common.hpp):
*   fluent      v.normalized().scaled(2.0).dot(w)
*   procedural  dot(scale(normalize(v), 2.0), w)
*
* FUNCTIONAL BRIDGE:
*   map(fn) / reduce(init, fn) mirror the functional subframework's vocabulary,
* so component-wise pipelines read the same as container pipelines:
*   v.map([](double c){ return c * c; }).sum()  ==  squared-magnitude.
*
* LEVELS:
*   Everything compiles from C++98. Operations are constexpr from C++14,
* where loops may be; those that take a root are too, on the C core's
* correctly rounded constant-expression root, the C library's bit for bit.
* An angle is the library's arc-cosine at run time at every level and a
* constant expression from C++20. A vector of up to four components is
* built from them at every level, of more from C++11 (or from an array at
* every level); std::array interoperates from C++11.
*
*
* path:      /inc/djinterp/math/linear_algebra/vector.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VECTOR
    ------
    1.  vector
2.  FREE FUNCTIONS
    --------------
    1.  Operators
    2.  Procedural spellings
3.  CONVENIENCE ALIASES
    -------------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_VECTOR_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_VECTOR_HPP 1

// std
#include <cstddef>                     // std::size_t
// djinterp
#include "../../djinterp.hpp"          // framework root
#include "./linalg_common.hpp"         // internal::linalg_kernel,
                                       // default_tolerance
// re_std
#include "../../../re_std/type_traits/enable_if.hpp"      // enable_if
#include "../../../re_std/type_traits/is_arithmetic.hpp"  // is_arithmetic
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                   // std::array
#endif


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  VECTOR
//==============================================================================


// 1.1    vector
//------------------------------------------------------------------------------
// vector
//   class: fixed-size column vector of N components of Type.
template<typename    Type,
         std::size_t N>
class vector
{
private:
    typedef internal::linalg_kernel<Type> kernel;

public:
    typedef Type        value_type;
    typedef std::size_t size_type;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    typedef std::array<Type, N> array_type;
#endif

    D_STATIC_ASSERT((N > 0), "vector: dimension must be at least 1.");

    // ---- construction -----------------------------------------------------

    // default: the zero vector.
    D_CONSTEXPR
    vector() D_NOEXCEPT
        : m_data()
    {}

    // from an array of N components (every level).
    D_CONSTEXPR_CPP14 explicit
    vector(
        const Type (&_components)[N]
    ) D_NOEXCEPT
        : m_data()
    {
        for (size_type i = 0; i < N; ++i)
        {
            m_data[i] = _components[i];
        }
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from a std::array of N components (C++11).
    D_CONSTEXPR_CPP14 explicit
    vector(
        const array_type& _components
    ) D_NOEXCEPT
        : m_data()
    {
        for (size_type i = 0; i < N; ++i)
        {
            m_data[i] = _components[i];
        }
    }
#endif

    // from exactly N arithmetic components, each converted to Type: one to
    // four at every level, as templates so a count that is not N, or a
    // component that is not arithmetic, removes them rather than failing.
    template<typename A0>
    D_CONSTEXPR_CPP14
    vector(
        A0 _x,
        typename re_std::enable_if<( (N == 1) &&
                                     re_std::is_arithmetic<A0>::value ),
                                   int>::type = 0
    ) D_NOEXCEPT
        : m_data()
    {
        m_data[0] = static_cast<Type>(_x);
    }

    template<typename A0,
             typename A1>
    D_CONSTEXPR_CPP14
    vector(
        A0 _x,
        A1 _y,
        typename re_std::enable_if<( (N == 2) &&
                                     re_std::is_arithmetic<A0>::value &&
                                     re_std::is_arithmetic<A1>::value ),
                                   int>::type = 0
    ) D_NOEXCEPT
        : m_data()
    {
        m_data[0] = static_cast<Type>(_x);
        m_data[1] = static_cast<Type>(_y);
    }

    template<typename A0,
             typename A1,
             typename A2>
    D_CONSTEXPR_CPP14
    vector(
        A0 _x,
        A1 _y,
        A2 _z,
        typename re_std::enable_if<( (N == 3) &&
                                     re_std::is_arithmetic<A0>::value &&
                                     re_std::is_arithmetic<A1>::value &&
                                     re_std::is_arithmetic<A2>::value ),
                                   int>::type = 0
    ) D_NOEXCEPT
        : m_data()
    {
        m_data[0] = static_cast<Type>(_x);
        m_data[1] = static_cast<Type>(_y);
        m_data[2] = static_cast<Type>(_z);
    }

    template<typename A0,
             typename A1,
             typename A2,
             typename A3>
    D_CONSTEXPR_CPP14
    vector(
        A0 _x,
        A1 _y,
        A2 _z,
        A3 _w,
        typename re_std::enable_if<( (N == 4) &&
                                     re_std::is_arithmetic<A0>::value &&
                                     re_std::is_arithmetic<A1>::value &&
                                     re_std::is_arithmetic<A2>::value &&
                                     re_std::is_arithmetic<A3>::value ),
                                   int>::type = 0
    ) D_NOEXCEPT
        : m_data()
    {
        m_data[0] = static_cast<Type>(_x);
        m_data[1] = static_cast<Type>(_y);
        m_data[2] = static_cast<Type>(_z);
        m_data[3] = static_cast<Type>(_w);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from exactly N arithmetic components, N above four (C++11).
    template<typename... Args,
             typename re_std::enable_if<
                 ( (sizeof...(Args) == N) &&
                   (N > 4) &&
                   internal::all_arithmetic<Args...>::value ),
                 int>::type = 0>
    D_CONSTEXPR_CPP14
    vector(
        Args... _args
    ) D_NOEXCEPT
        : m_data()
    {
        const Type values[] = { static_cast<Type>(_args)... };

        for (size_type i = 0; i < N; ++i)
        {
            m_data[i] = values[i];
        }
    }
#endif

    // ---- named factories --------------------------------------------------

    // zeros: the zero vector.
    static D_CONSTEXPR vector
    zeros() D_NOEXCEPT
    {
        return vector();
    }

    // filled: every component equal to _value.
    static D_CONSTEXPR_CPP14 vector
    filled(Type _value) D_NOEXCEPT
    {
        vector v;

        kernel::fill(v.m_data, N, _value);

        return v;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from_array: build from a std::array of components (C++11).
    static D_CONSTEXPR_CPP14 vector
    from_array(const array_type& _components) D_NOEXCEPT
    {
        return vector(_components);
    }
#endif

    // basis: the I-th standard basis vector (1 at I, 0 elsewhere).
    template<std::size_t I>
    static D_CONSTEXPR_CPP14 vector
    basis() D_NOEXCEPT
    {
        // vector::basis: the index must be below the dimension
        (void)internal::linalg_requires<(I < N)>::check();

        vector v;

        v.m_data[I] = static_cast<Type>(1);

        return v;
    }

    // ---- size / access ----------------------------------------------------

    static D_CONSTEXPR size_type
    size() D_NOEXCEPT
    {
        return N;
    }

    D_CONSTEXPR const Type&
    operator[](size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    D_CONSTEXPR_CPP14 Type&
    operator[](size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    D_CONSTEXPR const Type*
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    // named component accessors (only valid for sufficiently wide vectors).
    D_CONSTEXPR_CPP14 Type
    x() const D_NOEXCEPT
    {
        // vector::x: requires dimension >= 1
        (void)internal::linalg_requires<(N >= 1)>::check();

        return m_data[0];
    }

    D_CONSTEXPR_CPP14 Type
    y() const D_NOEXCEPT
    {
        // vector::y: requires dimension >= 2
        (void)internal::linalg_requires<(N >= 2)>::check();

        return m_data[1];
    }

    D_CONSTEXPR_CPP14 Type
    z() const D_NOEXCEPT
    {
        // vector::z: requires dimension >= 3
        (void)internal::linalg_requires<(N >= 3)>::check();

        return m_data[2];
    }

    D_CONSTEXPR_CPP14 Type
    w() const D_NOEXCEPT
    {
        // vector::w: requires dimension >= 4
        (void)internal::linalg_requires<(N >= 4)>::check();

        return m_data[3];
    }

    // with: a copy with component _i replaced by _value (immutable set).
    D_CONSTEXPR_CPP14 vector
    with(
        size_type _i,
        Type      _value
    ) const D_NOEXCEPT
    {
        vector v = *this;

        v.m_data[_i] = _value;

        return v;
    }

    // ---- element-wise arithmetic (fluent) ---------------------------------

    D_CONSTEXPR_CPP14 vector
    plus(const vector& _o) const D_NOEXCEPT
    {
        vector v;

        kernel::add(v.m_data, m_data, _o.m_data, N);

        return v;
    }

    D_CONSTEXPR_CPP14 vector
    minus(const vector& _o) const D_NOEXCEPT
    {
        vector v;

        kernel::sub(v.m_data, m_data, _o.m_data, N);

        return v;
    }

    D_CONSTEXPR_CPP14 vector
    scaled(Type _s) const D_NOEXCEPT
    {
        vector v;

        kernel::scale(v.m_data, m_data, _s, N);

        return v;
    }

    D_CONSTEXPR_CPP14 vector
    divided(Type _s) const D_NOEXCEPT
    {
        vector v;

        kernel::divide(v.m_data, m_data, _s, N);

        return v;
    }

    D_CONSTEXPR_CPP14 vector
    negated() const D_NOEXCEPT
    {
        vector v;

        kernel::negate(v.m_data, m_data, N);

        return v;
    }

    // hadamard: component-wise (Schur) product.
    D_CONSTEXPR_CPP14 vector
    hadamard(const vector& _o) const D_NOEXCEPT
    {
        vector v;

        kernel::hadamard(v.m_data, m_data, _o.m_data, N);

        return v;
    }

    // ---- products, norms, geometry ----------------------------------------

    D_CONSTEXPR_CPP14 Type
    dot(const vector& _o) const D_NOEXCEPT
    {
        return kernel::dot(m_data, _o.m_data, N);
    }

    // cross: 3-vector cross product (compile error if N != 3).
    D_CONSTEXPR_CPP14 vector
    cross(const vector& _o) const D_NOEXCEPT
    {
        // vector::cross: only defined for 3-vectors
        (void)internal::linalg_requires<(N == 3)>::check();

        vector v;

        kernel::cross(v.m_data, m_data, _o.m_data);

        return v;
    }

    D_CONSTEXPR_CPP14 Type
    norm_squared() const D_NOEXCEPT
    {
        return kernel::norm_squared(m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    norm() const D_NOEXCEPT
    {
        return kernel::norm(m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    length() const D_NOEXCEPT
    {
        return norm();
    }

    // normalized: this vector over its norm; the zero vector stays zero.
    D_CONSTEXPR_CPP14 vector
    normalized() const D_NOEXCEPT
    {
        const Type n = norm();

        return (n > static_cast<Type>(0)) ? divided(n) : zeros();
    }

    D_CONSTEXPR_CPP14 bool
    is_unit(Type _tol = default_tolerance<Type>()) const D_NOEXCEPT
    {
        return (internal::abs_c(norm() - static_cast<Type>(1)) <= _tol);
    }

    D_CONSTEXPR_CPP14 Type
    distance_squared(const vector& _o) const D_NOEXCEPT
    {
        return kernel::distance_squared(m_data, _o.m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    distance(const vector& _o) const D_NOEXCEPT
    {
        return kernel::distance(m_data, _o.m_data, N);
    }

    // projected_onto: the component along _o; onto the zero vector, zero.
    D_CONSTEXPR_CPP14 vector
    projected_onto(const vector& _o) const D_NOEXCEPT
    {
        vector v;

        kernel::project(v.m_data, m_data, _o.m_data, N);

        return v;
    }

    // rejected_from: what is left once the component along _o is removed.
    D_CONSTEXPR_CPP14 vector
    rejected_from(const vector& _o) const D_NOEXCEPT
    {
        vector v;

        kernel::reject(v.m_data, m_data, _o.m_data, N);

        return v;
    }

    // cos_angle: the cosine of the angle to _o; 0 when either is zero.
    D_CONSTEXPR_CPP14 Type
    cos_angle(const vector& _o) const D_NOEXCEPT
    {
        const Type d = norm() * _o.norm();

        return (d > static_cast<Type>(0)) ? (dot(_o) / d)
                                          : static_cast<Type>(0);
    }

    // angle_to: the angle to _o, in radians, its cosine clamped into [-1,
    // 1] so rounding past 1 gives 0. The C library's arc-cosine at run time,
    // a constant expression from C++20.
    D_CONSTEXPR_CPP20 Type
    angle_to(const vector& _o) const D_NOEXCEPT
    {
        const Type c = cos_angle(_o);

        return static_cast<Type>(
            internal::linalg_elementary<typename kernel::root_type>::acos(
                static_cast<typename kernel::root_type>(
                    (c > static_cast<Type>(1))  ? static_cast<Type>(1)  :
                    (c < static_cast<Type>(-1)) ? static_cast<Type>(-1) :
                                                  c)));
    }

    // lerp: (1 - t) this + t _o, each product rounded apart.
    D_CONSTEXPR_CPP14 vector
    lerp(
        const vector& _o,
        Type          _t
    ) const D_NOEXCEPT
    {
        vector v;

        kernel::lerp(v.m_data, m_data, _o.m_data, _t, N);

        return v;
    }

    // ---- functional bridge -----------------------------------------------

    // map: a new vector with _fn applied to each component.
    template<typename Fn>
    D_CONSTEXPR_CPP14 vector
    map(Fn _fn) const
    {
        vector v;

        for (size_type i = 0; i < N; ++i)
        {
            v.m_data[i] = static_cast<Type>(_fn(m_data[i]));
        }

        return v;
    }

    // reduce: a left fold of the components from _init.
    template<typename Acc,
             typename Fn>
    D_CONSTEXPR_CPP14 Acc
    reduce(
        Acc _init,
        Fn  _fn
    ) const
    {
        Acc acc = _init;

        for (size_type i = 0; i < N; ++i)
        {
            acc = _fn(acc, m_data[i]);
        }

        return acc;
    }

    // ---- reductions --------------------------------------------------------

    D_CONSTEXPR_CPP14 Type
    sum() const D_NOEXCEPT
    {
        return kernel::sum(m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    product() const D_NOEXCEPT
    {
        return kernel::product(m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    min_coeff() const D_NOEXCEPT
    {
        return kernel::min(m_data, N);
    }

    D_CONSTEXPR_CPP14 Type
    max_coeff() const D_NOEXCEPT
    {
        return kernel::max(m_data, N);
    }

    // equals: every component within _tol of _o's.
    D_CONSTEXPR_CPP14 bool
    equals(
        const vector& _o,
        Type          _tol = default_tolerance<Type>()
    ) const D_NOEXCEPT
    {
        return kernel::equals(m_data, _o.m_data, _tol, N);
    }

    // equal_to: every component == _o's (operator== spells it).
    D_CONSTEXPR_CPP14 bool
    equal_to(const vector& _o) const D_NOEXCEPT
    {
        return kernel::equal(m_data, _o.m_data, N);
    }

private:
    Type m_data[N];
};


//==============================================================================
// 2.  FREE FUNCTIONS
//==============================================================================


// 2.1    Operators
//------------------------------------------------------------------------------
// A scalar factor is any arithmetic type, converted to Type; the constraint
// is on the return type, which C++98 can spell.

template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
operator+(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b
) D_NOEXCEPT
{
    return _a.plus(_b);
}

template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
operator-(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b
) D_NOEXCEPT
{
    return _a.minus(_b);
}

template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
operator-(
    const vector<Type, N>& _a
) D_NOEXCEPT
{
    return _a.negated();
}

template<typename    Type,
         std::size_t N,
         typename    Scalar>
D_CONSTEXPR_CPP14
typename re_std::enable_if<re_std::is_arithmetic<Scalar>::value,
                           vector<Type, N> >::type
operator*(
    const vector<Type, N>& _v,
    Scalar                 _s
) D_NOEXCEPT
{
    return _v.scaled(static_cast<Type>(_s));
}

template<typename    Scalar,
         typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14
typename re_std::enable_if<re_std::is_arithmetic<Scalar>::value,
                           vector<Type, N> >::type
operator*(
    Scalar                 _s,
    const vector<Type, N>& _v
) D_NOEXCEPT
{
    return _v.scaled(static_cast<Type>(_s));
}

template<typename    Type,
         std::size_t N,
         typename    Scalar>
D_CONSTEXPR_CPP14
typename re_std::enable_if<re_std::is_arithmetic<Scalar>::value,
                           vector<Type, N> >::type
operator/(
    const vector<Type, N>& _v,
    Scalar                 _s
) D_NOEXCEPT
{
    return _v.divided(static_cast<Type>(_s));
}

template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 bool
operator==(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b
) D_NOEXCEPT
{
    return _a.equal_to(_b);
}

template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 bool
operator!=(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b
) D_NOEXCEPT
{
    return !(_a == _b);
}

// 2.2    Procedural spellings
//------------------------------------------------------------------------------
// Each delegates to its member; see the member for the meaning.

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
dot(const vector<Type, N>& _a, const vector<Type, N>& _b) D_NOEXCEPT
{
    return _a.dot(_b);
}

template<typename Type>
D_CONSTEXPR_CPP14 vector<Type, 3>
cross(const vector<Type, 3>& _a, const vector<Type, 3>& _b) D_NOEXCEPT
{
    return _a.cross(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
norm(const vector<Type, N>& _v) D_NOEXCEPT
{
    return _v.norm();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
length(const vector<Type, N>& _v) D_NOEXCEPT
{
    return _v.norm();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
norm_squared(const vector<Type, N>& _v) D_NOEXCEPT
{
    return _v.norm_squared();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
normalize(const vector<Type, N>& _v) D_NOEXCEPT
{
    return _v.normalized();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
scale(const vector<Type, N>& _v, Type _s) D_NOEXCEPT
{
    return _v.scaled(_s);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
distance(const vector<Type, N>& _a, const vector<Type, N>& _b) D_NOEXCEPT
{
    return _a.distance(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP20 Type
angle(const vector<Type, N>& _a, const vector<Type, N>& _b) D_NOEXCEPT
{
    return _a.angle_to(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
cos_angle(const vector<Type, N>& _a, const vector<Type, N>& _b) D_NOEXCEPT
{
    return _a.cos_angle(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
project(const vector<Type, N>& _v, const vector<Type, N>& _onto) D_NOEXCEPT
{
    return _v.projected_onto(_onto);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
reject(const vector<Type, N>& _v, const vector<Type, N>& _from) D_NOEXCEPT
{
    return _v.rejected_from(_from);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
hadamard(const vector<Type, N>& _a, const vector<Type, N>& _b) D_NOEXCEPT
{
    return _a.hadamard(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
lerp(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b,
    Type                   _t
) D_NOEXCEPT
{
    return _a.lerp(_b, _t);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
sum(const vector<Type, N>& _v) D_NOEXCEPT
{
    return _v.sum();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 bool
approx_equal(
    const vector<Type, N>& _a,
    const vector<Type, N>& _b,
    Type                   _tol = default_tolerance<Type>()
) D_NOEXCEPT
{
    return _a.equals(_b, _tol);
}


//==============================================================================
// 3.  CONVENIENCE ALIASES
//==============================================================================
// The alias templates are C++11's; the double ones are types at every level.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Type = double> using vector2 = vector<Type, 2>;
template<typename Type = double> using vector3 = vector<Type, 3>;
template<typename Type = double> using vector4 = vector<Type, 4>;
#endif

typedef vector<double, 2> vec2d;
typedef vector<double, 3> vec3d;
typedef vector<double, 4> vec4d;


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_VECTOR_HPP
