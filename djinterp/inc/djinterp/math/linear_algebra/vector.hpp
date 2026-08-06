/******************************************************************************
* djinterp [math]                                                   vector.hpp
*
* Fixed-size column vector for the linear-algebra subframework.
*   vector<_T, _N> stores its components by value in a std::array and every
* constructor, accessor, and operation is D_CONSTEXPR, so the same objects work
* at compile time and at runtime. Vectors are immutable in use: every operation
* returns a new vector rather than mutating in place, which is what lets them
* chain.
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
* NOTE (C++14):
*   The constexpr bodies use loops and local mutation (relaxed constexpr), the
* same baseline expression.hpp relies on.
*
* path:      /inc/djinterp/math/linear_algebra/vector.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_VECTOR_
#define DJINTERP_MATH_LINALG_VECTOR_ 1

// std
#include <cstddef>
#include <array>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./linalg_common.hpp"


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   VECTOR
// ===========================================================================

// vector
//   class: fixed-size column vector of _N components of type _T.
template<typename    _T,
         std::size_t _N>
class vector
{
    static_assert((_N > 0), "vector: dimension must be at least 1.");

public:
    using value_type = _T;
    using size_type  = std::size_t;
    using array_type = std::array<_T, _N>;

    // ---- construction -----------------------------------------------------

    // default: the zero vector.
    D_CONSTEXPR
    vector() noexcept
        : m_data{}
    {
    }

    // from an array of components. Reads the (const) std::array element-wise
    // -- const std::array::operator[] is constexpr even pre-C++17, while the
    // raw storage write is constexpr from C++14, so this stays compile-time.
    D_CONSTEXPR explicit
    vector(
        const array_type& _components
    ) noexcept
        : m_data{}
    {
        for (size_type i = 0; i < _N; ++i)
        {
            m_data[i] = _components[i];
        }
    }

    // from exactly _N scalar components, e.g. vector<double,3>(1.0, 2.0, 3.0).
    // constrained to arithmetic arguments so it never shadows copy/array forms.
    template<typename... _Args,
             typename std::enable_if<
                 ( (sizeof...(_Args) == _N) &&
                   internal::all_arithmetic<_Args...>::value ),
                 int>::type = 0>
    D_CONSTEXPR
    vector(
        _Args... _args
    ) noexcept
        : m_data{ static_cast<_T>(_args)... }
    {
    }

    // ---- named factories --------------------------------------------------

    // zeros: the zero vector.
    static D_CONSTEXPR vector
    zeros() noexcept
    {
        return vector();
    }

    // filled: every component equal to _value.
    static D_CONSTEXPR vector
    filled(_T _value) noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = _value;
        }

        return v;
    }

    // from_array: build from a std::array of components.
    static D_CONSTEXPR vector
    from_array(const array_type& _components) noexcept
    {
        return vector(_components);
    }

    // basis: the _I-th standard basis vector (1 at _I, 0 elsewhere).
    template<std::size_t _I>
    static D_CONSTEXPR vector
    basis() noexcept
    {
        static_assert((_I < _N), "vector::basis: index out of range.");

        vector v;
        v.m_data[_I] = static_cast<_T>(1);

        return v;
    }

    // ---- size / access ----------------------------------------------------

    static D_CONSTEXPR size_type
    size() noexcept
    {
        return _N;
    }

    D_CONSTEXPR const _T&
    operator[](size_type _i) const noexcept
    {
        return m_data[_i];
    }

    D_CONSTEXPR _T&
    operator[](size_type _i) noexcept
    {
        return m_data[_i];
    }

    D_CONSTEXPR const _T*
    data() const noexcept
    {
        return m_data;
    }

    // named component accessors (only valid for sufficiently wide vectors).
    D_CONSTEXPR _T
    x() const noexcept
    {
        static_assert((_N >= 1), "vector::x: requires dimension >= 1.");

        return m_data[0];
    }

    D_CONSTEXPR _T
    y() const noexcept
    {
        static_assert((_N >= 2), "vector::y: requires dimension >= 2.");

        return m_data[1];
    }

    D_CONSTEXPR _T
    z() const noexcept
    {
        static_assert((_N >= 3), "vector::z: requires dimension >= 3.");

        return m_data[2];
    }

    D_CONSTEXPR _T
    w() const noexcept
    {
        static_assert((_N >= 4), "vector::w: requires dimension >= 4.");

        return m_data[3];
    }

    // with: a copy with component _i replaced by _value (immutable set).
    D_CONSTEXPR vector
    with(
        size_type _i,
        _T        _value
    ) const noexcept
    {
        vector v       = *this;
        v.m_data[_i]   = _value;

        return v;
    }

    // ---- element-wise arithmetic (fluent) ---------------------------------

    D_CONSTEXPR vector
    plus(const vector& _o) const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = m_data[i] + _o.m_data[i];
        }

        return v;
    }

    D_CONSTEXPR vector
    minus(const vector& _o) const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = m_data[i] - _o.m_data[i];
        }

        return v;
    }

    D_CONSTEXPR vector
    scaled(_T _s) const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = m_data[i] * _s;
        }

        return v;
    }

    D_CONSTEXPR vector
    divided(_T _s) const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = m_data[i] / _s;
        }

        return v;
    }

    D_CONSTEXPR vector
    negated() const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = -m_data[i];
        }

        return v;
    }

    // hadamard: component-wise (Schur) product.
    D_CONSTEXPR vector
    hadamard(const vector& _o) const noexcept
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = m_data[i] * _o.m_data[i];
        }

        return v;
    }

    // ---- products, norms, geometry ----------------------------------------

    D_CONSTEXPR _T
    dot(const vector& _o) const noexcept
    {
        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < _N; ++i)
        {
            acc += m_data[i] * _o.m_data[i];
        }

        return acc;
    }

    // cross: 3-vector cross product (compile error if _N != 3).
    D_CONSTEXPR vector
    cross(const vector& _o) const noexcept
    {
        static_assert((_N == 3), "vector::cross: only defined for 3-vectors.");

        return vector(
            m_data[1] * _o.m_data[2] - m_data[2] * _o.m_data[1],
            m_data[2] * _o.m_data[0] - m_data[0] * _o.m_data[2],
            m_data[0] * _o.m_data[1] - m_data[1] * _o.m_data[0]);
    }

    D_CONSTEXPR _T
    norm_squared() const noexcept
    {
        return dot(*this);
    }

    // norm / length: Euclidean magnitude (constexpr via the local kernel).
    D_CONSTEXPR _T
    norm() const noexcept
    {
        return static_cast<_T>(
            internal::sqrt_c(static_cast<double>(norm_squared())));
    }

    D_CONSTEXPR _T
    length() const noexcept
    {
        return norm();
    }

    // normalized: unit vector in the same direction (zero vector -> zeros).
    D_CONSTEXPR vector
    normalized() const noexcept
    {
        const _T n = norm();

        return (n > static_cast<_T>(0)) ? divided(n) : zeros();
    }

    D_CONSTEXPR bool
    is_unit(_T _tol = default_tolerance<_T>()) const noexcept
    {
        return ( internal::abs_c(norm() - static_cast<_T>(1)) <= _tol );
    }

    D_CONSTEXPR _T
    distance_squared(const vector& _o) const noexcept
    {
        return minus(_o).norm_squared();
    }

    D_CONSTEXPR _T
    distance(const vector& _o) const noexcept
    {
        return minus(_o).norm();
    }

    // projected_onto: component of *this along _o.
    D_CONSTEXPR vector
    projected_onto(const vector& _o) const noexcept
    {
        const _T d = _o.norm_squared();

        return (d > static_cast<_T>(0)) ? _o.scaled(dot(_o) / d) : zeros();
    }

    // rejected_from: component of *this orthogonal to _o.
    D_CONSTEXPR vector
    rejected_from(const vector& _o) const noexcept
    {
        return minus(projected_onto(_o));
    }

    // cos_angle: cosine of the angle to _o (exact; no transcendental needed).
    D_CONSTEXPR _T
    cos_angle(const vector& _o) const noexcept
    {
        const _T d = norm() * _o.norm();

        return (d > static_cast<_T>(0)) ? (dot(_o) / d) : static_cast<_T>(0);
    }

    // angle_to: angle to _o in radians (uses the approximate constexpr acos).
    D_CONSTEXPR _T
    angle_to(const vector& _o) const noexcept
    {
        return static_cast<_T>(
            internal::acos_c(static_cast<double>(cos_angle(_o))));
    }

    // lerp: linear interpolation, (1 - _t) * this + _t * _o.
    D_CONSTEXPR vector
    lerp(
        const vector& _o,
        _T            _t
    ) const noexcept
    {
        return scaled(static_cast<_T>(1) - _t).plus(_o.scaled(_t));
    }

    // ---- functional-style reductions / maps -------------------------------

    // map: a new vector with _fn applied to each component.
    template<typename _Fn>
    D_CONSTEXPR vector
    map(_Fn _fn) const
    {
        vector v;

        for (size_type i = 0; i < _N; ++i)
        {
            v.m_data[i] = static_cast<_T>(_fn(m_data[i]));
        }

        return v;
    }

    // reduce: left fold of _fn over the components starting from _init.
    template<typename _Acc,
             typename _Fn>
    D_CONSTEXPR _Acc
    reduce(
        _Acc _init,
        _Fn  _fn
    ) const
    {
        _Acc acc = _init;

        for (size_type i = 0; i < _N; ++i)
        {
            acc = _fn(acc, m_data[i]);
        }

        return acc;
    }

    D_CONSTEXPR _T
    sum() const noexcept
    {
        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < _N; ++i)
        {
            acc += m_data[i];
        }

        return acc;
    }

    D_CONSTEXPR _T
    product() const noexcept
    {
        _T acc = static_cast<_T>(1);

        for (size_type i = 0; i < _N; ++i)
        {
            acc *= m_data[i];
        }

        return acc;
    }

    D_CONSTEXPR _T
    min_coeff() const noexcept
    {
        _T m = m_data[0];

        for (size_type i = 1; i < _N; ++i)
        {
            if (m_data[i] < m)
            {
                m = m_data[i];
            }
        }

        return m;
    }

    D_CONSTEXPR _T
    max_coeff() const noexcept
    {
        _T m = m_data[0];

        for (size_type i = 1; i < _N; ++i)
        {
            if (m_data[i] > m)
            {
                m = m_data[i];
            }
        }

        return m;
    }

    // ---- comparison -------------------------------------------------------

    // equals: component-wise within _tol.
    D_CONSTEXPR bool
    equals(
        const vector& _o,
        _T            _tol = default_tolerance<_T>()
    ) const noexcept
    {
        for (size_type i = 0; i < _N; ++i)
        {
            if (internal::abs_c(m_data[i] - _o.m_data[i]) > _tol)
            {
                return false;
            }
        }

        return true;
    }

private:
    // raw array storage: a built-in subscript write is a constant expression
    // from C++14, unlike std::array::operator[] (non-const), which is only
    // constexpr from C++17. array_type (std::array) is retained above purely as
    // the ergonomic component type for the factories/operators.
    _T m_data[_N];
};


// ===========================================================================
// II.  OPERATORS
// ===========================================================================

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
operator+(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.plus(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
operator-(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.minus(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
operator-(
    const vector<_T, _N>& _a
) noexcept
{
    return _a.negated();
}

// scalar on the right: v * s.
template<typename    _T,
         std::size_t _N,
         typename    _S,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
D_CONSTEXPR vector<_T, _N>
operator*(
    const vector<_T, _N>& _v,
    _S                    _s
) noexcept
{
    return _v.scaled(static_cast<_T>(_s));
}

// scalar on the left: s * v.
template<typename    _S,
         typename    _T,
         std::size_t _N,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
D_CONSTEXPR vector<_T, _N>
operator*(
    _S                    _s,
    const vector<_T, _N>& _v
) noexcept
{
    return _v.scaled(static_cast<_T>(_s));
}

template<typename    _T,
         std::size_t _N,
         typename    _S,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
D_CONSTEXPR vector<_T, _N>
operator/(
    const vector<_T, _N>& _v,
    _S                    _s
) noexcept
{
    return _v.divided(static_cast<_T>(_s));
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
operator==(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        if (!(_a[i] == _b[i]))
        {
            return false;
        }
    }

    return true;
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
operator!=(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return !(_a == _b);
}


// ===========================================================================
// III. FREE FUNCTIONS  (procedural spelling; delegate to the members)
// ===========================================================================

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
dot(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.dot(_b);
}

template<typename _T>
D_CONSTEXPR vector<_T, 3>
cross(
    const vector<_T, 3>& _a,
    const vector<_T, 3>& _b
) noexcept
{
    return _a.cross(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
norm(const vector<_T, _N>& _v) noexcept
{
    return _v.norm();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
length(const vector<_T, _N>& _v) noexcept
{
    return _v.norm();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
norm_squared(const vector<_T, _N>& _v) noexcept
{
    return _v.norm_squared();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
normalize(const vector<_T, _N>& _v) noexcept
{
    return _v.normalized();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
scale(
    const vector<_T, _N>& _v,
    _T                    _s
) noexcept
{
    return _v.scaled(_s);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
distance(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.distance(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
angle(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.angle_to(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
cos_angle(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.cos_angle(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
project(
    const vector<_T, _N>& _v,
    const vector<_T, _N>& _onto
) noexcept
{
    return _v.projected_onto(_onto);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
reject(
    const vector<_T, _N>& _v,
    const vector<_T, _N>& _from
) noexcept
{
    return _v.rejected_from(_from);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
hadamard(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b
) noexcept
{
    return _a.hadamard(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
lerp(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b,
    _T                    _t
) noexcept
{
    return _a.lerp(_b, _t);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
sum(const vector<_T, _N>& _v) noexcept
{
    return _v.sum();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
approx_equal(
    const vector<_T, _N>& _a,
    const vector<_T, _N>& _b,
    _T                    _tol = default_tolerance<_T>()
) noexcept
{
    return _a.equals(_b, _tol);
}


// ===========================================================================
// IV.  CONVENIENCE ALIASES
// ===========================================================================

template<typename _T = double> using vector2 = vector<_T, 2>;
template<typename _T = double> using vector3 = vector<_T, 3>;
template<typename _T = double> using vector4 = vector<_T, 4>;

using vec2d = vector<double, 2>;
using vec3d = vector<double, 3>;
using vec4d = vector<double, 4>;

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_VECTOR_
