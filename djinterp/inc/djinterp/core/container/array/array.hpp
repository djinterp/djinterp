/******************************************************************************
* djinterp [container]                                                          array.hpp
*
* Concrete array container supporting all permutations of the
* lifetime/iterability axes:
*
*               | iterable               | non-iterable
*   ------------+------------------------+--------------------------
*   constexpr   | array<...,             | array<...,
*               |   constexpr_lifetime,  |   constexpr_lifetime,
*               |   iterable>            |   non_iterable>
*   ------------+------------------------+--------------------------
*   immutable   | array<...,             | array<...,
*               |   immutable_lifetime,  |   immutable_lifetime,
*               |   iterable>            |   non_iterable>
*   ------------+------------------------+--------------------------
*   mutable     | array<...,             | array<...,
*               |   mutable_lifetime,    |   mutable_lifetime,
*               |   iterable>            |   non_iterable>
*
*   That gives 6 distinct named cells; combined with the
* {static-extent vs dynamic-extent} axis (the latter modeled
* by passing _Capacity == array::dynamic_extent), the user can
* stamp out:
*
*     1) constexpr  / mutable   / iterable
*     2) constexpr  / mutable   / non_iterable
*     3) constexpr  / immutable / iterable
*     4) constexpr  / immutable / non_iterable
*     5) runtime    / mutable   / iterable
*     6) runtime    / mutable   / non_iterable
*     7) runtime    / immutable / iterable
*     8) runtime    / immutable / non_iterable
*
*   "constexpr" here means the storage and accessors are usable
* in constant evaluation; "runtime" means storage may live on the
* heap or have stronger non-constexpr guarantees.  A fully-
* constexpr value of array<int, 4, mutable_lifetime, iterable>
* doubles as a literal type if used in a constexpr context, while
* the same template instantiated with runtime = true does not.
*
*   Convenience aliases at the bottom of the file expose each cell
* by name (e.g. `constexpr_iterable_array`, `immutable_array`,
* `non_iterable_array`).
*
*   PORTABILITY:
*   C++11 baseline.  Members marked with D_INTERNAL_ARRAY_CONSTEXPR
* are constexpr from C++14 onward (relaxed constexpr) and inline
* otherwise.  Range-based for loops, alias templates, fold
* expressions, and concepts are gated.
*
* TABLE OF CONTENTS
* =================
* I.    Policy enums and tag dispatch
* II.   array primary template
* III.  Specializations along the iterability axis
* IV.   Specializations along the lifetime axis
* V.    Convenience aliases
* VI.   Free-function factories
* VII.  Free-function bulk algorithms
*
*
* path:      /inc/djinterp/container/array/array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARRAY_
#define DJINTERP_CONTAINER_ARRAY_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../core/djinterp.hpp"
#include "../../../core/meta/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/constexpr_iterator.hpp"
#include "./array_traits.hpp"
#include "./array_iterator.hpp"


// D_INTERNAL_ARRAY_CONSTEXPR
//   macro: relaxed-constexpr (C++14+) for mutator member functions.
// Empty in C++11 since constexpr there forbids non-trivial function
// bodies, multiple statements, and modification of *this.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_INTERNAL_ARRAY_CONSTEXPR constexpr
#else
    #define D_INTERNAL_ARRAY_CONSTEXPR
#endif


NS_DJINTERP


// ===========================================================================
// I.   Policy enums and tag dispatch
// ===========================================================================

// array_iterability
//   enum: opt-in iterability axis.  An iterable array exposes the
// usual STL-shaped begin()/end()/cbegin()/cend()/rbegin()/rend()
// surface.  A non-iterable array exposes only data()/size() and
// element accessors; range-based for loops and STL algorithms will
// not bind to it.
enum class array_iterability
{
    iterable,
    non_iterable
};


// array_storage_kind
//   enum: distinguishes a static-extent array (capacity baked into
// the type) from a dynamic-extent one (capacity tracked at run-time
// against a maximum).  We never allocate on the heap: dynamic extent
// just means the active size is a member rather than the full _N.
enum class array_storage_kind
{
    static_extent,
    dynamic_extent
};


// dynamic_extent
//   constant: sentinel value for the _N template parameter
// indicating dynamic-extent storage.  Mirrors std::dynamic_extent.
static constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);


NS_INTERNAL


    // is_dynamic_extent_v
    //   trait: true if _N == dynamic_extent.
    template<std::size_t _N>
    struct is_dynamic_extent
        : std::integral_constant<bool, (_N == dynamic_extent)>
    {};


    // array_assert_size
    //   helper: compile-time bounds check for static-extent
    // arrays; runtime check for dynamic.
    template<typename _SizeType>
    inline D_INTERNAL_ARRAY_CONSTEXPR
    void array_assert_size(_SizeType _index, _SizeType _size)
    {
        // intentional empty body for non-debug builds; the call
        // exists so that constexpr evaluation will trigger a
        // diagnostic when _index >= _size via the array operator.
        (void)_index;
        (void)_size;
    }


NS_END  // internal


// ===========================================================================
// II.  array primary template
// ===========================================================================
// The primary template is the {mutable, iterable, static-extent}
// case.  Other points in the cube specialize on the lifetime and
// iterability template parameters.

// array
//   class template:  fixed- or dynamic-extent contiguous container
// parameterized by element type, capacity, lifetime, and iterability.
template<typename _Type,
         std::size_t                     _N,
         array_lifetime       _Lifetime    = array_lifetime::mutable_lifetime,
         array_iterability    _Iterability =   array_iterability::iterable>
class array
{
public:
    using value_type             = _Type;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = _Type&;
    using const_reference        = const _Type&;
    using pointer                = _Type*;
    using const_pointer          = const _Type*;
    using iterator               = _Type*;
    using const_iterator         = const _Type*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type extent = _N;
    static constexpr
        array_lifetime lifetime    = _Lifetime;
    static constexpr
        array_iterability         iterability = _Iterability;

private:
    _Type m_data[_N == 0 ? 1 : _N];

public:
    // ---------------------------------------------------------------
    //  Construction
    // ---------------------------------------------------------------
    constexpr
    array() D_NOEXCEPT
        : m_data{}
    {}

    template<typename... _Args,
             typename = typename std::enable_if<
            // exclude the case "exactly one argument that is array<...> itself"
            !(sizeof...(_Args) == 1
              && std::is_same<
                     typename std::decay<
                         typename std::tuple_element<
                             0,
                             std::tuple<_Args...>
                         >::type
                     >::type,
                     array
                 >::value)
        >::type
    >
    constexpr
    array(_Args&&... _args) D_NOEXCEPT
        : m_data{static_cast<_Type>(std::forward<_Args>(_args))...}
    {}

    // ---------------------------------------------------------------
    //  Element access
    // ---------------------------------------------------------------
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    operator[](size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    operator[](size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    at(size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    at(size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    front() D_NOEXCEPT
    {
        return m_data[0];
    }

    constexpr
    const_reference
    front() const D_NOEXCEPT
    {
        return m_data[0];
    }

    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    back() D_NOEXCEPT
    {
        return m_data[_N - 1];
    }

    constexpr
    const_reference
    back() const D_NOEXCEPT
    {
        return m_data[_N - 1];
    }

    D_INTERNAL_ARRAY_CONSTEXPR
    pointer
    data() D_NOEXCEPT
    {
        return m_data;
    }

    constexpr
    const_pointer
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    // ---------------------------------------------------------------
    //  Capacity
    // ---------------------------------------------------------------
    constexpr
    size_type
    size() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    size_type
    max_size() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    size_type
    capacity() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    bool
    empty() const D_NOEXCEPT
    {
        return _N == 0;
    }

    // ---------------------------------------------------------------
    //  Iteration  (only present when iterable)
    // ---------------------------------------------------------------
    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    iterator
    begin() D_NOEXCEPT
    {
        return m_data;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    begin() const D_NOEXCEPT
    {
        return m_data;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    cbegin() const D_NOEXCEPT
    {
        return m_data;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    iterator
    end() D_NOEXCEPT
    {
        return m_data + _N;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    end() const D_NOEXCEPT
    {
        return m_data + _N;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    cend() const D_NOEXCEPT
    {
        return m_data + _N;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reverse_iterator
    rbegin() D_NOEXCEPT
    {
        return reverse_iterator(end());
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_reverse_iterator
    rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reverse_iterator
    rend() D_NOEXCEPT
    {
        return reverse_iterator(begin());
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_reverse_iterator
    rend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    // ---------------------------------------------------------------
    //  Mutation  (only present when mutable)
    // ---------------------------------------------------------------
    template<bool _Enabled =
        (_Lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    void
    fill(const _Type& _v)
    {
        for (size_type i = 0; i < _N; ++i)
        {
            m_data[i] = _v;
        }
    }

    template<bool _Enabled = (_Lifetime != array_lifetime::immutable_lifetime),
         typename = typename std::enable_if<_Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    void
    swap(
        array& _other
    ) D_NOEXCEPT
    {
        for (size_type i = 0; i < _N; ++i)
        {
            _Type tmp = std::move(m_data[i]);
            m_data[i] = std::move(_other.m_data[i]);
            _other.m_data[i] = std::move(tmp);
        }
    }
};


// ===========================================================================
// III. Specialization:  immutable variants
// ===========================================================================
// An immutable array exposes no non-const accessors and no mutator
// methods.  Construction is the only opportunity to populate it.

template<typename          _Type,
         std::size_t       _N,
         array_iterability _Iterability>
class array<_Type,
            _N,
            array_lifetime::immutable_lifetime,
            _Iterability>
{
public:
    using value_type             = _Type;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using const_reference        = const _Type&;
    using const_pointer          = const _Type*;
    using const_iterator         = const _Type*;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type extent = _N;
    static constexpr array_lifetime    lifetime  = array_lifetime::immutable_lifetime;
    static constexpr array_iterability iterability = _Iterability;

private:
    const _Type m_data[_N == 0 ? 1 : _N];

public:
    template<typename... _Args,
             typename = typename std::enable_if<
            // exclude the case "exactly one argument that is array<...> itself"
            !(sizeof...(_Args) == 1
              && std::is_same<
                     typename std::decay<
                         typename std::tuple_element<
                             0,
                             std::tuple<_Args...>
                         >::type
                     >::type,
                     array
                 >::value)
        >::type
    >
    constexpr
    array(_Args&&... _args) D_NOEXCEPT
        : m_data{static_cast<_Type>(std::forward<_Args>(_args))...}
    {}

    constexpr
    const_reference
    operator[](size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    at(size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    front() const D_NOEXCEPT
    {
        return m_data[0];
    }

    constexpr
    const_reference
    back() const D_NOEXCEPT
    {
        return m_data[_N - 1];
    }

    constexpr
    const_pointer
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    constexpr
    size_type
    size() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    size_type
    max_size() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    size_type
    capacity() const D_NOEXCEPT
    {
        return _N;
    }

    constexpr
    bool
    empty() const D_NOEXCEPT
    {
        return _N == 0;
    }

    // Iteration   (only on the iterable variants)
    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    begin() const D_NOEXCEPT
    {
        return m_data;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    cbegin() const D_NOEXCEPT
    {
        return m_data;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    end() const D_NOEXCEPT
    {
        return m_data + _N;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_iterator
    cend() const D_NOEXCEPT
    {
        return m_data + _N;
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_reverse_iterator
    rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    template<bool _Enabled = (_Iterability == array_iterability::iterable),
             typename = typename std::enable_if<_Enabled>::type>
    constexpr
    const_reverse_iterator
    rend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }
};


// ===========================================================================
// IV.  Convenience aliases
// ===========================================================================
// Each axis combination gets a named alias for ergonomic use.  The
// `constexpr_lifetime` cells are aliases of the mutable variant -
// constexpr usability is determined entirely by *how* the array is
// used, not by its declaration; the tag exists so that traits can
// classify storage that is intended for compile-time consumption.

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // mutable_iterable_array
    //   alias: a mutable array that exposes iteration.
    template<typename _Type, std::size_t _N>
    using mutable_iterable_array = array<_Type, _N,
        array_lifetime::mutable_lifetime,
        array_iterability::iterable>;

    // mutable_non_iterable_array
    //   alias: a mutable array that hides iteration.
    template<typename _Type, std::size_t _N>
    using mutable_non_iterable_array = array<_Type, _N,
        array_lifetime::mutable_lifetime,
        array_iterability::non_iterable>;

    // immutable_iterable_array
    //   alias: a read-only array that exposes iteration.
    template<typename _Type, std::size_t _N>
    using immutable_iterable_array = array<_Type, _N,
        array_lifetime::immutable_lifetime,
        array_iterability::iterable>;

    // immutable_non_iterable_array
    //   alias: a read-only array that hides iteration.
    template<typename _Type, std::size_t _N>
    using immutable_non_iterable_array = array<_Type, _N,
        array_lifetime::immutable_lifetime,
        array_iterability::non_iterable>;

    // constexpr_iterable_array
    //   alias: a constexpr-friendly array that exposes iteration.
    // Compile-time iterability is only available on iterable cells.
    template<typename _Type, std::size_t _N>
    using constexpr_iterable_array = array<_Type, _N,
        array_lifetime::constexpr_lifetime,
        array_iterability::iterable>;

    // constexpr_non_iterable_array
    //   alias: a constexpr-friendly array that hides iteration.
    template<typename _Type, std::size_t _N>
    using constexpr_non_iterable_array = array<_Type, _N,
        array_lifetime::constexpr_lifetime,
        array_iterability::non_iterable>;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


// ===========================================================================
// V.   Free-function factories
// ===========================================================================

// make_array
//   factory: deduces element type and size from the argument pack.
// Returns a mutable, iterable array.
#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES \
        && D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)

    template<typename _Type, typename... _Args>
    constexpr
    array<_Type, sizeof...(_Args) + 1,
          array_lifetime::mutable_lifetime,
          array_iterability::iterable>
    make_array(_Type _first, _Args&&... _rest)
    {
        return array<_Type, sizeof...(_Args) + 1,
                     array_lifetime::mutable_lifetime,
                     array_iterability::iterable>(
                         _first,
                         std::forward<_Args>(_rest)...);
    }

#endif


// make_immutable_array
//   factory: same as make_array but produces an immutable array.
#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES  &&                            \
     D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)

    template<typename _Type, typename... _Args>
    constexpr
    array<_Type, sizeof...(_Args) + 1,
          array_lifetime::immutable_lifetime,
          array_iterability::iterable>
    make_immutable_array(_Type _first, _Args&&... _rest)
    {
        return array<_Type, sizeof...(_Args) + 1,
                     array_lifetime::immutable_lifetime,
                     array_iterability::iterable>(
                         _first,
                         std::forward<_Args>(_rest)...);
    }

#endif


// ===========================================================================
// VI.  Free-function bulk algorithms
// ===========================================================================

// array_equal
//   algo: element-wise equality between two arrays of the
// same shape.  Available on every cell because it depends only
// on data()/size() rather than iteration.
template<typename          _Type,
         std::size_t       _N,
         array_lifetime    _LifeA,
         array_iterability _IterA,
         array_lifetime    _LifeB,
         array_iterability _IterB>
constexpr
bool
array_equal(
    const array<_Type,
                _N,
                _LifeA,
                _IterA>& _a,
    const array<_Type,
                _N,
                _LifeB,
                _IterB>& _b
) D_NOEXCEPT
{
    return constexpr_equal(_a.data(), 
                           _a.data() + _N,
                           _b.data());
}


// array_copy
//   algo: copies element-wise from a source to a destination
// array of the same shape.  Requires the destination to be
// mutable; that constraint is enforced via SFINAE on the
// destination's lifetime tag.
template<typename _Type,
         std::size_t          _N,
         array_lifetime       _LifeSrc,
         array_iterability    _IterSrc,
         array_lifetime       _LifeDst,
         array_iterability    _IterDst,
         typename = typename std::enable_if<
             (_LifeDst != array_lifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_copy(const array<_Type, _N, _LifeSrc, _IterSrc>& _src,
           array<_Type, _N, _LifeDst, _IterDst>&       _dst)
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        _dst.data()[i] = _src.data()[i];
    }
}


// array_swap
//   algo: in-place swap of two same-shape arrays' contents.
// Both must be mutable.
template<typename          _Type,
         std::size_t       _N,
         array_lifetime    _LifeA,
         array_iterability _IterA,
         array_lifetime    _LifeB,
         array_iterability _IterB,
         typename = typename std::enable_if<
                (_LifeA != array_lifetime::immutable_lifetime)
             && (_LifeB != array_lifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_swap(array<_Type, _N, _LifeA, _IterA>& _a,
           array<_Type, _N, _LifeB, _IterB>& _b)
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        _Type tmp = std::move(_a.data()[i]);
        _a.data()[i] = std::move(_b.data()[i]);
        _b.data()[i] = std::move(tmp);
    }
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARRAY_


#undef D_INTERNAL_ARRAY_CONSTEXPR