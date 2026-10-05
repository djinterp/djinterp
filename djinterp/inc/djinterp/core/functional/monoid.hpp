/*******************************************************************************
* djinterp [core]                                                     monoid.hpp
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
*     - sum<T>         : addition,                identity 0.
*     - product<T>     : multiplication,          identity 1.
*     - all            : logical AND,             identity true.
*     - any            : logical OR,              identity false.
*     - min<T>         : minimum,                 identity +inf (max).
*     - max<T>         : maximum,                 identity -inf (lowest).
*   The newtypes live in namespace monoids so they do not collide with the
* accumulator factories (sum / min / max / mean) that already exist flat in
* djinterp; a scalar is a monoid in more than one way, so the wrapper names the
* intended one. Each wraps a public `value`.
*
* USAGE:
*   using namespace djinterp;
*   auto total = mconcat(std::vector<sum<int> >{
*                    sum<int>(1), sum<int>(2),
*                    sum<int>(3) }).value;                    // 6
*
*   // fold any Foldable through a monoid:
*   maybe<int> m = just(5);
*   int s = fold_monoid(m, [](int x){ return sum<int>(x); }).value;  // 5
*
*   std::string j = mconcat(std::vector<std::string>{ "a", "b", "c" });       // "abc"
*
*
* path:      /inc/djinterp/core/functional/monoid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.11
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    MONOID NEWTYPES                              (namespace monoids)
      ----------------------------------------------------------------
      1.    sum<T> / product<T>
      2.    all / any
      3.    min<T> / max<T>

II.   MONOID PROTOCOL
      ---------------
      1.    monoid_traits<T>                         (primary, undefined)
      2.    is_monoid<T>                             (detection trait)

III.  INSTANCES                                     (semigroup + monoid)
      ------------------------------------------------------------------
      1.    std::string, std::vector<T>
      2.    the  newtypes

IV.   GENERIC MONOID OPERATIONS
      -------------------------
      1.    mempty<T>                                (identity element)
      2.    mconcat                                  (combine a foldable of M)
      3.    fold_monoid                              (map into M, then mconcat)
*/


#ifndef DJINTERP_FUNCTIONAL_MONOID_HPP
#define DJINTERP_FUNCTIONAL_MONOID_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
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

// sum
//   struct: the additive monoid over Type -- combine is +, identity 0.
template<typename Type>
struct sum
{
    Type value;

    D_CONSTEXPR
    sum()
        : value(Type())
    {}

    D_CONSTEXPR
    explicit sum(
        Type _value
    )
        : value(_value)
    {}
};

// product
//   struct: the multiplicative monoid over Type -- combine is *,
// identity 1.
template<typename Type>
struct product
{
    Type value;

    D_CONSTEXPR
    product()
        : value(Type(1))
    {}

    D_CONSTEXPR
    explicit product(
        Type _value
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
//   struct: the minimum monoid over Type -- combine keeps the smaller,
// identity is the largest representable Type. Intended for numeric
// Type (the identity is std::numeric_limits<Type>::max()).
template<typename Type>
struct min
{
    Type value;

    D_CONSTEXPR
    explicit min(
        Type _value
    )
        : value(_value)
    {}
};

// max
//   struct: the maximum monoid over Type -- combine keeps the larger,
// identity is the smallest representable Type. Intended for numeric
// Type (the identity is std::numeric_limits<Type>::lowest()).
template<typename Type>
struct max
{
    Type value;

    D_CONSTEXPR
    explicit max(
        Type _value
    )
        : value(_value)
    {}
};

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
template<typename Monoid,
         typename Enable = void>
struct monoid_traits;


NS_INTERNAL

    // is_monoid_helper
    //   helper: SFINAE detector for whether monoid_traits<T> is specialized.
    // Looks for the is_specialized marker that every specialization provides.
    template<typename Type>
    struct is_monoid_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename monoid_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

NS_END  // internal


// is_monoid
//   trait: true if Type has a specialization of monoid_traits (after cv-ref
// stripping). A monoid is necessarily a semigroup, so is_semigroup is also
// true for any such type.
template<typename Type>
struct is_monoid
    : internal::is_monoid_helper<typename std::decay<Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_monoid_v
//   value: convenience alias for is_monoid<Type>::value.
template<typename Type>
static constexpr bool is_monoid_v = is_monoid<Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Monoid
    //   concept: satisfied when Type is a specialized monoid. The PascalCase
    // typeclass face, alongside Semigroup / Functor / Applicative / Foldable.
    template<typename Type>
    concept Monoid = is_monoid<Type>::value;

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

// semigroup_traits<std::vector<Type>>
//   instance: vector concatenation is associative.
template<typename Type>
struct semigroup_traits<std::vector<Type>, void>
{
    using is_specialized = std::true_type;

    static
    std::vector<Type> combine(
        const std::vector<Type>& _a,
        const std::vector<Type>& _b
    )
    {
        std::vector<Type> _result;

        _result.reserve(_a.size() + _b.size());
        _result.insert(_result.end(), _a.begin(), _a.end());
        _result.insert(_result.end(), _b.begin(), _b.end());

        return _result;
    }
};

// monoid_traits<std::vector<Type>>
//   instance: the empty vector is the identity for concatenation.
template<typename Type>
struct monoid_traits<std::vector<Type>, void>
{
    using is_specialized = std::true_type;

    static
    std::vector<Type> empty()
    {
        return std::vector<Type>();
    }
};


// -- sum<T> : addition ---------------------------------------------

template<typename Type>
struct semigroup_traits<sum<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    sum<Type> combine(
        const sum<Type>& _a,
        const sum<Type>& _b
    )
    {
        return sum<Type>(_a.value + _b.value);
    }
};

template<typename Type>
struct monoid_traits<sum<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    sum<Type> empty()
    {
        return sum<Type>();
    }
};


// -- product<T> : multiplication -----------------------------------

template<typename Type>
struct semigroup_traits<product<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    product<Type> combine(
        const product<Type>& _a,
        const product<Type>& _b
    )
    {
        return product<Type>(_a.value * _b.value);
    }
};

template<typename Type>
struct monoid_traits<product<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    product<Type> empty()
    {
        return product<Type>();
    }
};


// -- all : logical AND ---------------------------------------------

template<>
struct semigroup_traits<all, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    all combine(
        const all& _a,
        const all& _b
    )
    {
        return all(_a.value && _b.value);
    }
};

template<>
struct monoid_traits<all, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    all empty()
    {
        return all();
    }
};


// -- any : logical OR ----------------------------------------------

template<>
struct semigroup_traits<any, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    any combine(
        const any& _a,
        const any& _b
    )
    {
        return any(_a.value || _b.value);
    }
};

template<>
struct monoid_traits<any, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    any empty()
    {
        return any();
    }
};


// -- min<T> : minimum ----------------------------------------------

template<typename Type>
struct semigroup_traits<min<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    min<Type> combine(
        const min<Type>& _a,
        const min<Type>& _b
    )
    {
        return (_b.value < _a.value) ? _b : _a;
    }
};

template<typename Type>
struct monoid_traits<min<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    min<Type> empty()
    {
        return min<Type>((std::numeric_limits<Type>::max)());
    }
};


// -- max<T> : maximum ----------------------------------------------

template<typename Type>
struct semigroup_traits<max<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    max<Type> combine(
        const max<Type>& _a,
        const max<Type>& _b
    )
    {
        return (_a.value < _b.value) ? _b : _a;
    }
};

template<typename Type>
struct monoid_traits<max<Type>, void>
{
    using is_specialized = std::true_type;

    static
    D_CONSTEXPR
    max<Type> empty()
    {
        return max<Type>((std::numeric_limits<Type>::lowest)());
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
//   function: the identity element of a monoid. The monoid type Monoid must
// be supplied explicitly because it cannot be deduced (the dual of how
// monad_unit / pure take their type explicitly).
//
//   Example: mempty<sum<int>>().value -> 0
template<typename Monoid>
D_NODISCARD
D_CONSTEXPR
Monoid mempty()
{
    return monoid_traits<Monoid>::empty();
}


NS_INTERNAL

    // monoid_mappend_helper
    //   helper: the reducer behind mconcat / fold_monoid -- combines the
    // running accumulator with the next monoid value via mappend, threading
    // the accumulator by value. A named functor keeps it usable on every
    // floor and lets the trailing return types name it.
    template<typename Monoid>
    struct monoid_mappend_helper
    {
        D_CONSTEXPR
        Monoid operator()(
            Monoid         _acc,
            const Monoid& _value
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
//   Example: mconcat(vector<sum<int>>{1,2,3}).value -> 6
template<typename Foldable>
D_NODISCARD
D_CONSTEXPR
foldable_value_type_t<Foldable>
mconcat
(
    const Foldable& _fa
)
{
    using monoid_t = foldable_value_type_t<Foldable>;

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
//   Example: fold_monoid(just(5), [](int x){ return sum<int>(x); })
//            -> sum<int> with value 5
template<typename Foldable,
         typename Function>
D_NODISCARD
D_CONSTEXPR
typename std::decay<decltype(std::declval<Function&>()(
    std::declval<const foldable_value_type_t<Foldable>&>()))>::type
fold_monoid
(
    const Foldable& _fa,
    Function         _function
)
{
    using value_t  = foldable_value_type_t<Foldable>;
    using monoid_t = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const value_t&>()))>::type;

    return ::djinterp::fold_left(
        _fa,
        ::djinterp::mempty<monoid_t>(),
        [_function](monoid_t _acc, const value_t& _element) -> monoid_t
        {
            return ::djinterp::mappend(_acc, _function(_element));
        });
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_MONOID_HPP
