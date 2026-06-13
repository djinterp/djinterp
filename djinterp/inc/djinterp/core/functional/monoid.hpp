/******************************************************************************
* djinterp [functional]                                             monoid.hpp
*
* Monoid protocol, its identity-aware operations, and the standard instances.
*   A monoid is a semigroup (an associative combine, semigroup.hpp) that also
* has an identity element -- a value mempty such that combine(mempty, x) ==
* combine(x, mempty) == x. From the pair (combine, mempty) a whole collection
* of values can be reduced to one: mconcat folds a Foldable of monoid values,
* and fold_monoid maps each element of any Foldable into a monoid and folds.
* This is where Foldable and the algebra of combining meet.
*
*   Because C++ has no native type classes, a monoid is recognized by
* specializing monoid_traits<T> with a single static empty (the combine comes
* from its semigroup_traits). mempty<T>() returns the identity (T explicit,
* since it cannot be deduced); mconcat / fold_monoid then collapse a Foldable.
*
*   The standard instances live here, each defining a type's full algebra --
* its semigroup combine and its monoid identity -- in one place:
*     - std::string            : concatenation,           identity "".
*     - std::vector<T>          : concatenation,           identity {}.
*     - monoids::sum<T>         : addition,                identity 0.
*     - monoids::product<T>     : multiplication,          identity 1.
*     - monoids::all            : logical AND,             identity true.
*     - monoids::any            : logical OR,              identity false.
*     - monoids::min<T>         : minimum,                 identity +inf (max).
*     - monoids::max<T>         : maximum,                 identity -inf (lowest).
*   The newtypes live in namespace monoids so they do not collide with the
* accumulator factories (sum / min / max / mean) that already exist flat in
* djinterp; a scalar is a monoid in more than one way, so the wrapper names the
* intended one. Each wraps a public `value`.
*
* USAGE:
*   using namespace djinterp;
*   auto total = mconcat(std::vector<monoids::sum<int> >{
*                    monoids::sum<int>(1), monoids::sum<int>(2),
*                    monoids::sum<int>(3) }).value;                    // 6
*
*   // fold any Foldable through a monoid:
*   maybe<int> m = just(5);
*   int s = fold_monoid(m, [](int x){ return monoids::sum<int>(x); }).value;  // 5
*
*   std::string j = mconcat(std::vector<std::string>{ "a", "b", "c" });       // "abc"
*
* 
* path:      /inc/djinterp/core/functional/monoid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    MONOID NEWTYPES                              (namespace monoids)
      1.  sum<T> / product<T>
      2.  all / any
      3.  min<T> / max<T>
II.   MONOID PROTOCOL
      1.  monoid_traits<T>                         (primary, undefined)
      2.  is_monoid<T>                             (detection trait)
III.  INSTANCES                                     (semigroup + monoid)
      1.  std::string, std::vector<T>
      2.  the monoids:: newtypes
IV.   GENERIC MONOID OPERATIONS
      1.  mempty<T>                                (identity element)
      2.  mconcat                                  (combine a foldable of M)
      3.  fold_monoid                              (map into M, then mconcat)
*/


#ifndef DJINTERP_FUNCTIONAL_MONOID_
#define DJINTERP_FUNCTIONAL_MONOID_ 1

// std
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./semigroup.hpp"
#include "./foldable.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    MONOID NEWTYPES  (namespace monoids)                  ///
///////////////////////////////////////////////////////////////////////////////
//   A scalar carries more than one monoid (int combines under + and under *;
// bool under && and under ||), so the operation cannot be read off the type
// alone. These newtypes name the intended one. Each wraps a public `value`;
// the namespace keeps them clear of the flat accumulator factories.

namespace monoids
{

    // sum
    //   struct: the additive monoid over _Type -- combine is +, identity 0.
    template<typename _Type>
    struct sum
    {
        _Type value;

        D_CONSTEXPR
        sum()
            : value(_Type())
        {}

        D_CONSTEXPR
        explicit sum(
            _Type _value
        )
            : value(_value)
        {}
    };

    // product
    //   struct: the multiplicative monoid over _Type -- combine is *,
    // identity 1.
    template<typename _Type>
    struct product
    {
        _Type value;

        D_CONSTEXPR
        product()
            : value(_Type(1))
        {}

        D_CONSTEXPR
        explicit product(
            _Type _value
        )
            : value(_value)
        {}
    };

    // all
    //   struct: the conjunctive monoid over bool -- combine is &&,
    // identity true.
    struct all
    {
        bool value;

        D_CONSTEXPR
        all()
            : value(true)
        {}

        D_CONSTEXPR
        explicit all(
            bool _value
        )
            : value(_value)
        {}
    };

    // any
    //   struct: the disjunctive monoid over bool -- combine is ||,
    // identity false.
    struct any
    {
        bool value;

        D_CONSTEXPR
        any()
            : value(false)
        {}

        D_CONSTEXPR
        explicit any(
            bool _value
        )
            : value(_value)
        {}
    };

    // min
    //   struct: the minimum monoid over _Type -- combine keeps the smaller,
    // identity is the largest representable _Type. Intended for numeric
    // _Type (the identity is std::numeric_limits<_Type>::max()).
    template<typename _Type>
    struct min
    {
        _Type value;

        D_CONSTEXPR
        explicit min(
            _Type _value
        )
            : value(_value)
        {}
    };

    // max
    //   struct: the maximum monoid over _Type -- combine keeps the larger,
    // identity is the smallest representable _Type. Intended for numeric
    // _Type (the identity is std::numeric_limits<_Type>::lowest()).
    template<typename _Type>
    struct max
    {
        _Type value;

        D_CONSTEXPR
        explicit max(
            _Type _value
        )
            : value(_value)
        {}
    };

}  // namespace monoids


///////////////////////////////////////////////////////////////////////////////
///             II.   MONOID PROTOCOL                                       ///
///////////////////////////////////////////////////////////////////////////////

// monoid_traits
//   trait: primary template, undefined by default. Each concrete monoid
// specializes monoid_traits<T> to expose:
//
//     - empty()        : static T empty() -- the identity element
//     - is_specialized = true_type (marker)
//
//   A monoid is also a semigroup: its combine comes from semigroup_traits<T>
// (every instance below specializes both). The second parameter is a SFINAE
// hook, mirroring semigroup_traits. The primary is left undefined so a use on
// a non-monoid produces a clean resolution error.
template<typename _Monoid,
         typename _Enable = void>
struct monoid_traits;


NS_INTERNAL

    // is_monoid_helper
    //   helper: SFINAE detector for whether monoid_traits<T> is specialized.
    // Looks for the is_specialized marker that every specialization provides.
    template<typename _Type>
    struct is_monoid_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename monoid_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_monoid
//   trait: true if _Type has a specialization of monoid_traits (after cv-ref
// stripping). A monoid is necessarily a semigroup, so is_semigroup is also
// true for any such type.
template<typename _Type>
struct is_monoid
    : internal::is_monoid_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_monoid_v
//   value: convenience alias for is_monoid<_Type>::value.
template<typename _Type>
static constexpr bool is_monoid_v = is_monoid<_Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Monoid
    //   concept: satisfied when _Type is a specialized monoid. The PascalCase
    // typeclass face, alongside Semigroup / Functor / Applicative / Foldable.
    template<typename _Type>
    concept Monoid = is_monoid<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             III.  INSTANCES  (semigroup + monoid)                       ///
///////////////////////////////////////////////////////////////////////////////
//   Each type's full algebra in one place: a semigroup_traits (combine) and a
// monoid_traits (identity). All are written in the explicit two-argument
// `<T, void>` specialization form against the SFINAE-hooked primaries.

// -- std::string : concatenation --------------------------------------------

// semigroup_traits<std::string>
//   instance: string concatenation is associative.
template<>
struct semigroup_traits<std::string, void>
{
    using is_specialized = std::true_type;

    static
    std::string combine(
        const std::string& _a,
        const std::string& _b
    )
    {
        return _a + _b;
    }
};

// monoid_traits<std::string>
//   instance: the empty string is the identity for concatenation.
template<>
struct monoid_traits<std::string, void>
{
    using is_specialized = std::true_type;

    static
    std::string empty()
    {
        return std::string();
    }
};


// -- std::vector<T> : concatenation -----------------------------------------

// semigroup_traits<std::vector<_Type>>
//   instance: vector concatenation is associative.
template<typename _Type>
struct semigroup_traits<std::vector<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    std::vector<_Type> combine(
        const std::vector<_Type>& _a,
        const std::vector<_Type>& _b
    )
    {
        std::vector<_Type> _result;

        _result.reserve(_a.size() + _b.size());
        _result.insert(_result.end(), _a.begin(), _a.end());
        _result.insert(_result.end(), _b.begin(), _b.end());

        return _result;
    }
};

// monoid_traits<std::vector<_Type>>
//   instance: the empty vector is the identity for concatenation.
template<typename _Type>
struct monoid_traits<std::vector<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    std::vector<_Type> empty()
    {
        return std::vector<_Type>();
    }
};


// -- monoids::sum<T> : addition ---------------------------------------------

template<typename _Type>
struct semigroup_traits<monoids::sum<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::sum<_Type> combine(
        const monoids::sum<_Type>& _a,
        const monoids::sum<_Type>& _b
    )
    {
        return monoids::sum<_Type>(_a.value + _b.value);
    }
};

template<typename _Type>
struct monoid_traits<monoids::sum<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::sum<_Type> empty()
    {
        return monoids::sum<_Type>();
    }
};


// -- monoids::product<T> : multiplication -----------------------------------

template<typename _Type>
struct semigroup_traits<monoids::product<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::product<_Type> combine(
        const monoids::product<_Type>& _a,
        const monoids::product<_Type>& _b
    )
    {
        return monoids::product<_Type>(_a.value * _b.value);
    }
};

template<typename _Type>
struct monoid_traits<monoids::product<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::product<_Type> empty()
    {
        return monoids::product<_Type>();
    }
};


// -- monoids::all : logical AND ---------------------------------------------

template<>
struct semigroup_traits<monoids::all, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::all combine(
        const monoids::all& _a,
        const monoids::all& _b
    )
    {
        return monoids::all(_a.value && _b.value);
    }
};

template<>
struct monoid_traits<monoids::all, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::all empty()
    {
        return monoids::all();
    }
};


// -- monoids::any : logical OR ----------------------------------------------

template<>
struct semigroup_traits<monoids::any, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::any combine(
        const monoids::any& _a,
        const monoids::any& _b
    )
    {
        return monoids::any(_a.value || _b.value);
    }
};

template<>
struct monoid_traits<monoids::any, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::any empty()
    {
        return monoids::any();
    }
};


// -- monoids::min<T> : minimum ----------------------------------------------

template<typename _Type>
struct semigroup_traits<monoids::min<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::min<_Type> combine(
        const monoids::min<_Type>& _a,
        const monoids::min<_Type>& _b
    )
    {
        return (_b.value < _a.value) ? _b : _a;
    }
};

template<typename _Type>
struct monoid_traits<monoids::min<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::min<_Type> empty()
    {
        return monoids::min<_Type>((std::numeric_limits<_Type>::max)());
    }
};


// -- monoids::max<T> : maximum ----------------------------------------------

template<typename _Type>
struct semigroup_traits<monoids::max<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::max<_Type> combine(
        const monoids::max<_Type>& _a,
        const monoids::max<_Type>& _b
    )
    {
        return (_a.value < _b.value) ? _b : _a;
    }
};

template<typename _Type>
struct monoid_traits<monoids::max<_Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    monoids::max<_Type> empty()
    {
        return monoids::max<_Type>((std::numeric_limits<_Type>::lowest)());
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   GENERIC MONOID OPERATIONS                             ///
///////////////////////////////////////////////////////////////////////////////
//   mempty delegates to monoid_traits<T>::empty. mconcat and fold_monoid fold
// a Foldable (foldable.hpp) through the monoid, threading mempty as the seed
// and mappend (semigroup.hpp) as the reducer. They are D_CONSTEXPR and fold at
// compile time wherever the underlying combine / fold_left do (C++20 over a
// carrier-holding maybe / result), and run at runtime otherwise.

// mempty
//   function: the identity element of a monoid. The monoid type _Monoid must
// be supplied explicitly because it cannot be deduced (the dual of how
// monad_unit / pure take their type explicitly).
//
//   Example: mempty<monoids::sum<int>>().value -> 0
template<typename _Monoid>
D_NODISCARD
D_CONSTEXPR
_Monoid mempty()
{
    return monoid_traits<_Monoid>::empty();
}


NS_INTERNAL

    // monoid_mappend_helper
    //   helper: the reducer behind mconcat / fold_monoid -- combines the
    // running accumulator with the next monoid value via mappend, threading
    // the accumulator by value. A named functor keeps it usable on every
    // floor and lets the trailing return types name it.
    template<typename _Monoid>
    struct monoid_mappend_helper
    {
        D_CONSTEXPR
        _Monoid operator()(
            _Monoid        _acc,
            const _Monoid& _value
        ) const
        {
            return ::djinterp::mappend(_acc, _value);
        }
    };

NS_END  // internal


// mconcat
//   function: combines every element of a Foldable whose elements are
// themselves a monoid, into a single value -- folding from mempty with
// mappend. The empty foldable yields mempty.
//
//   Example: mconcat(vector<monoids::sum<int>>{1,2,3}).value -> 6
template<typename _Foldable>
D_NODISCARD
D_CONSTEXPR
foldable_value_type_t<_Foldable>
mconcat
(
    const _Foldable& _fa
)
{
    using monoid_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        ::djinterp::mempty<monoid_t>(),
        internal::monoid_mappend_helper<monoid_t>());
}


// fold_monoid
//   function: maps each element of a Foldable into a monoid via _function,
// then combines them (mconcat after a map). The monoid is deduced from the
// result of _function, so -- unlike fold_map -- no identity or combine need be
// supplied: they come from the monoid protocol.
//
//   Example: fold_monoid(just(5), [](int x){ return monoids::sum<int>(x); })
//            -> sum<int> with value 5
template<typename _Foldable,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
typename std::decay<decltype(std::declval<_Function&>()(
    std::declval<const foldable_value_type_t<_Foldable>&>()))>::type
fold_monoid
(
    const _Foldable& _fa,
    _Function        _function
)
{
    using value_t  = foldable_value_type_t<_Foldable>;
    using monoid_t = typename std::decay<decltype(
        std::declval<_Function&>()(std::declval<const value_t&>()))>::type;

    return ::djinterp::fold_left(
        _fa,
        ::djinterp::mempty<monoid_t>(),
        [_function](monoid_t _acc, const value_t& _element) -> monoid_t
        {
            return ::djinterp::mappend(_acc, _function(_element));
        });
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_MONOID_
