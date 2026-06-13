/******************************************************************************
* djinterp [functional]                                         comparator.hpp
*
* First-class comparators with composable combinators (C++).
*   A comparator is a binary callable returning true when its first
* argument is "less than" its second, matching std::sort / std::set /
* std::map conventions. This module elevates comparators to first-class
* values: built from key functions, reversed, chained as tie-breakers,
* and lifted across types.
*
*   DUAL-STANDARD: the C++11+ implementation (perfect forwarding,
* D_CONSTEXPR, decltype) is the primary path. A C++98 fallback is
* provided under #else: const-ref construction, no constexpr, fixed
* result_type typedefs in place of decltype, and explicit adapter
* types as the RHS of operator|. Both paths expose identical symbols
* in djinterp. See the `cpp98 roadmap` workbook.
*
* 
* path:      /inc/djinterp/core/functional/comparator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMPARATOR_HPP_
#define DJINTERP_FUNCTIONAL_COMPARATOR_HPP_ 1

// std
#include <utility>
// djinterp
#include "../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#  include <type_traits>
#  include "./functional_traits.hpp"
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
//   C++11+ PRIMARY IMPLEMENTATION                                            //
///////////////////////////////////////////////////////////////////////////////


NS_DJINTERP


//   DUAL DOMAIN.  Comparators lift uniformly.  The combinators (reversed,
// then / then_by tie-break chains, by_key / by_member / by_function, lifted)
// are D_CONSTEXPR and compose at compile time, and because a comparator is just
// a constexpr binary callable, applying one folds during constant evaluation:
// over ordinary values for compile-time ordering decisions, and over carrier
// leaves (val_t / type_t) when paired with a key extractor that reads the
// carrier (e.g. by_key([](auto c){ return c.value; })).  The same comparator
// then serves at run time as a std::sort / std::set / std::map predicate, so
// one description drives both a constant-evaluated ordering and a runtime one.

///////////////////////////////////////////////////////////////////////////////
///             I.    PRIMITIVE COMPARATORS                                 ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // natural_helper
    //   helper: comparator that uses operator< on the supplied
    // type. The default comparator; equivalent to std::less<_T>.
    template<typename _T>
    struct natural_helper
    {
        D_CONSTEXPR
        bool operator()(
            const _T& _a,
            const _T& _b
        ) const
        {
            return (_a < _b);
        }
    };


    // by_key_helper
    //   helper: extracts a key from each argument via _KeyFn and
    // compares the extracted keys with operator<. Captures the
    // key function by value so it can be invoked twice per call.
    template<typename _KeyFn>
    class by_key_helper
    {
    public:
        // Constrained so this forwarding constructor does not hijack
        // copy/move construction: without the enable_if, composing two
        // by_key comparators (e.g. then(c1, c2), which copies a
        // by_key_helper) would route the copy through here and try to
        // treat the other helper as a key-function. (fixed 2026-05-30)
        template<typename _KFnFwd,
                 typename std::enable_if<
                     !std::is_same<
                         typename std::decay<_KFnFwd>::type,
                         by_key_helper>::value,
                     int>::type = 0>
        explicit D_CONSTEXPR
        by_key_helper(
            _KFnFwd&& _key_fn
        )
            : m_key_fn(std::forward<_KFnFwd>(_key_fn))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return (m_key_fn(_a) < m_key_fn(_b));
        }

    private:
        _KeyFn m_key_fn;
    };


    // by_function_helper
    //   helper: thin wrapper around an arbitrary binary callable
    // returning bool. Provided so all comparator factories yield
    // the same helper-style structure; useful when a user already
    // has a comparator-shaped lambda and wants to chain it.
    template<typename _Fn>
    class by_function_helper
    {
    public:
        template<typename _FFwd>
        explicit D_CONSTEXPR
        by_function_helper(
            _FFwd&& _fn
        )
            : m_fn(std::forward<_FFwd>(_fn))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_fn(_a, _b);
        }

    private:
        _Fn m_fn;
    };


    // member_accessor
    //   helper: callable that reads a data-member of _Class via
    // a stored pointer-to-member. Used by comparators::by_member
    // to build a by_key_helper without a stateful lambda (which
    // is awkward to spell in C++11 return-type contexts).
    template<typename _Class,
             typename _Member>
    class member_accessor
    {
    public:
        explicit D_CONSTEXPR
        member_accessor(
            _Member _Class::* _ptr
        )
            : m_ptr(_ptr)
        {}

        D_CONSTEXPR
        const _Member& operator()(
            const _Class& _obj
        ) const
        {
            return _obj.*m_ptr;
        }

    private:
        _Member _Class::* m_ptr;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   COMBINATOR HELPERS                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // reversed_helper
    //   helper: comparator that returns the inverse ordering of
    // _Inner. Implemented by swapping the argument order rather
    // than negating the result; this preserves strict-weak-
    // ordering semantics even when _Inner is partial (treats
    // a == b as not (a < b) and not (b < a)).
    template<typename _Inner>
    class reversed_helper
    {
    public:
        template<typename _IFwd>
        explicit D_CONSTEXPR
        reversed_helper(
            _IFwd&& _inner
        )
            : m_inner(std::forward<_IFwd>(_inner))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_inner(_b, _a);
        }

    private:
        _Inner m_inner;
    };


    // then_helper
    //   helper: tie-breaker chain. Compares with _Primary; if the
    // arguments are equivalent under _Primary (i.e. neither
    // primary(a, b) nor primary(b, a) is true), defers to
    // _Secondary. This is the standard lexicographic combination
    // and chains naturally: c1 | then(c2 | then(c3)).
    template<typename _Primary,
             typename _Secondary>
    class then_helper
    {
    public:
        template<typename _PFwd,
                 typename _SFwd>
        D_CONSTEXPR
        then_helper(
            _PFwd&& _primary,
            _SFwd&& _secondary
        )
            : m_primary(std::forward<_PFwd>(_primary))
            , m_secondary(std::forward<_SFwd>(_secondary))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            if (m_primary(_a, _b))
            {
                return true;
            }

            if (m_primary(_b, _a))
            {
                return false;
            }

            return m_secondary(_a, _b);
        }

    private:
        _Primary   m_primary;
        _Secondary m_secondary;
    };


    // lifted_helper
    //   helper: composes a comparator with a key function so the
    // resulting comparator operates on a different type. The
    // key function is applied to each argument; the inner
    // comparator is invoked on the extracted keys.
    template<typename _Comparator,
             typename _KeyFn>
    class lifted_helper
    {
    public:
        template<typename _CFwd,
                 typename _KFwd>
        D_CONSTEXPR
        lifted_helper(
            _CFwd&& _comparator,
            _KFwd&& _key_fn
        )
            : m_comparator(std::forward<_CFwd>(_comparator))
            , m_key_fn(std::forward<_KFwd>(_key_fn))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_comparator(m_key_fn(_a), m_key_fn(_b));
        }

    private:
        _Comparator m_comparator;
        _KeyFn      m_key_fn;
    };


    // then_adapter
    //   helper: rhs of operator|. Stores a secondary comparator;
    // when piped against a primary, builds a then_helper. This is
    // the standard adapter pattern reused from view.hpp /
    // monad.hpp.
    template<typename _Secondary>
    class then_adapter
    {
    public:
        template<typename _SFwd>
        explicit D_CONSTEXPR
        then_adapter(
            _SFwd&& _secondary
        )
            : m_secondary(std::forward<_SFwd>(_secondary))
        {}

        template<typename _Primary>
        D_CONSTEXPR
        then_helper<typename std::decay<_Primary>::type, _Secondary>
        apply(
            _Primary&& _primary
        ) const
        {
            return then_helper<
                typename std::decay<_Primary>::type, _Secondary>(
                    std::forward<_Primary>(_primary), m_secondary);
        }

    private:
        _Secondary m_secondary;
    };


    // reversed_adapter
    //   helper: rhs of operator| that wraps the lhs in a
    // reversed_helper. Lets users write `cmp | reversed()` as a
    // pipeline alternative to `reversed(cmp)`.
    struct reversed_adapter
    {
        template<typename _Inner>
        D_CONSTEXPR
        reversed_helper<typename std::decay<_Inner>::type>
        apply(
            _Inner&& _inner
        ) const
        {
            return reversed_helper<
                typename std::decay<_Inner>::type>(
                    std::forward<_Inner>(_inner));
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

namespace comparators
{

    // natural
    //   function: builds a comparator that uses operator< on
    // the supplied type. The default ordering; equivalent to
    // std::less<_T>{} but expressed in the framework's style.
    template<typename _T>
    D_NODISCARD D_CONSTEXPR internal::natural_helper<_T>
    natural()
    {
        return internal::natural_helper<_T>{};
    }


    // by_key
    //   function: builds a comparator that extracts a key from
    // each argument via _key_fn and compares the keys with
    // operator<. _key_fn may be a free function, lambda, or
    // member function pointer.
    //
    //   For pointer-to-data-member targets, prefer by_member,
    // which has better readability at call sites.
    template<typename _KeyFn>
    D_NODISCARD D_CONSTEXPR internal::by_key_helper<typename std::decay<_KeyFn>::type>
    by_key(
        _KeyFn&& _key_fn
    )
    {
        return internal::by_key_helper<
            typename std::decay<_KeyFn>::type>(
                std::forward<_KeyFn>(_key_fn));
    }


    // by_member
    //   function: builds a comparator that compares two objects
    // by a data-member pointer. Equivalent to by_key with a
    // closure that reads the member, but reads more naturally
    // at call sites and avoids the C++11 lambda-in-return-type
    // limitation (lambdas in unevaluated contexts are C++20).
    //
    //   Example: by_member(&Person::age)
    template<typename _Class,
             typename _Member>
    D_NODISCARD D_CONSTEXPR internal::by_key_helper<internal::member_accessor<_Class, _Member>>
    by_member(
        _Member _Class::* _member_ptr
    )
    {
        return internal::by_key_helper<internal::member_accessor<_Class, _Member>>(internal::member_accessor<_Class, _Member>(_member_ptr));
    }


    // by_function
    //   function: builds a comparator from an arbitrary binary
    // callable returning bool. Useful as an adapter so that a
    // raw lambda comparator can be chained with the combinators
    // in this module (then, reversed, etc.).
    template<typename _Fn>
    D_NODISCARD D_CONSTEXPR internal::by_function_helper<typename std::decay<_Fn>::type>
    by_function(
        _Fn&& _fn
    )
    {
        return internal::by_function_helper<typename std::decay<_Fn>::type>(std::forward<_Fn>(_fn));
    }


    // reversed
    //   function: wraps a comparator to produce the inverse
    // ordering. May also be invoked as a pipeline RHS:
    // `cmp | reversed()` (no arguments) yields an adapter that
    // wraps its lhs.
    template<typename _Comparator>
    D_NODISCARD D_CONSTEXPR internal::reversed_helper<typename std::decay<_Comparator>::type>
    reversed(
        _Comparator&& _comparator
    )
    {
        return internal::reversed_helper<
            typename std::decay<_Comparator>::type>(
                std::forward<_Comparator>(_comparator));
    }


    // reversed (no-arg, adapter form)
    //   function: builds a pipeline adapter so `cmp | reversed()`
    // wraps cmp in a reversed_helper. Overload resolution picks
    // this form when no comparator is supplied.
    inline
    internal::reversed_adapter
    reversed()
    {
        return internal::reversed_adapter{};
    }


    // then
    //   function: builds a tie-breaker chain. then(c1, c2)
    // returns a comparator that compares with c1; when c1
    // reports equivalence, falls back to c2. Pipeline form:
    // c1 | then(c2).
    template<typename _Primary,
             typename _Secondary>
    D_NODISCARD D_CONSTEXPR internal::then_helper<typename std::decay<_Primary>::type,
                          typename std::decay<_Secondary>::type>
    then(
        _Primary&&   _primary,
        _Secondary&& _secondary
    )
    {
        return internal::then_helper<
            typename std::decay<_Primary>::type,
            typename std::decay<_Secondary>::type>(
                std::forward<_Primary>(_primary),
                std::forward<_Secondary>(_secondary));
    }


    // then (single-arg, adapter form)
    //   function: builds a pipeline adapter so `c1 | then(c2)`
    // produces a then_helper. The adapter holds the secondary;
    // operator| invokes its apply method with the primary on
    // the left-hand side.
    template<typename _Secondary>
    D_NODISCARD D_CONSTEXPR internal::then_adapter<typename std::decay<_Secondary>::type>
    then(
        _Secondary&& _secondary
    )
    {
        return internal::then_adapter<
            typename std::decay<_Secondary>::type>(
                std::forward<_Secondary>(_secondary));
    }


    // lifted
    //   function: composes a comparator with a key function,
    // producing a comparator that operates on a different type.
    // Equivalent to by_key with the comparison delegated to
    // _comparator rather than operator<.
    //
    //   Example:
    //     auto people_cmp = lifted(natural<int>(),
    //                              [](const Person& p) { return p.age; });
    template<typename _Comparator,
             typename _KeyFn>
    D_NODISCARD D_CONSTEXPR internal::lifted_helper<typename std::decay<_Comparator>::type,
                            typename std::decay<_KeyFn>::type>
    lifted(
        _Comparator&& _comparator,
        _KeyFn&&      _key_fn
    )
    {
        return internal::lifted_helper<
            typename std::decay<_Comparator>::type,
            typename std::decay<_KeyFn>::type>(
                std::forward<_Comparator>(_comparator),
                std::forward<_KeyFn>(_key_fn));
    }

}   // namespace comparators


///////////////////////////////////////////////////////////////////////////////
///             IV.   PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_comparator_adapter
    //   helper: SFINAE detector for comparator adapters (objects
    // with an apply(comparator) method). Used to constrain the
    // pipeline operator|.
    template<typename _Type,
             typename _Probe>
    struct is_comparator_adapter_helper
    {
    private:
        template<typename _T, typename _P>
        static auto test(int) -> decltype(
            std::declval<const _T&>().apply(std::declval<_P>()),
            std::true_type{});

        template<typename, typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type, _Probe>(0));
    };

NS_END  // internal


// NOTE: the previous generic `operator|(comparator, adapter)` here was
// constrained only on "adapter has apply(comparator)", which also matched
// unrelated apply-able RHS types (view / producer terminals), making
// `view | to_vector()` ambiguous whenever comparator.hpp and view.hpp were
// included together. It is replaced below by two tightly-typed overloads
// that fire only for this module's own adapters. (fixed 2026-05-30)

// operator| (comparator | then_adapter)
//   forwards a comparator into a then_adapter to build a tie-breaker
// chain. The concrete then_adapter RHS prevents this from firing on
// unrelated operands.
template<typename _Primary,
         typename _Secondary>
D_CONSTEXPR
internal::then_helper<typename std::decay<_Primary>::type, _Secondary>
operator|(
    _Primary&&                                _primary,
    const internal::then_adapter<_Secondary>& _adapter
)
{
    return _adapter.apply(std::forward<_Primary>(_primary));
}

// operator| (comparator | reversed_adapter)
//   wraps the comparator in a reversed_helper.
template<typename _Primary>
D_CONSTEXPR
internal::reversed_helper<typename std::decay<_Primary>::type>
operator|(
    _Primary&&                        _primary,
    const internal::reversed_adapter& _adapter
)
{
    return _adapter.apply(std::forward<_Primary>(_primary));
}


///////////////////////////////////////////////////////////////////////////////
///             V.    DERIVED PREDICATES                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // equal_under_helper
    //   helper: derives an equality predicate from a comparator
    // using the fact that, under strict-weak-ordering, a == b iff
    // !(a < b) && !(b < a).
    template<typename _Comparator>
    class equal_under_helper
    {
    public:
        template<typename _CFwd>
        explicit D_CONSTEXPR
        equal_under_helper(
            _CFwd&& _comparator
        )
            : m_comparator(std::forward<_CFwd>(_comparator))
        {}

        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return !m_comparator(_a, _b) && !m_comparator(_b, _a);
        }

    private:
        _Comparator m_comparator;
    };


    // less_than_helper / greater_than_helper
    //   helpers: one-sided binders that bind one argument of a
    // comparator, yielding a unary predicate. less_than(cmp, x)
    // gives "v < x", greater_than(cmp, x) gives "x < v" (i.e.
    // "v > x" under cmp).
    template<typename _Comparator,
             typename _Bound>
    class less_than_helper
    {
    public:
        template<typename _CFwd,
                 typename _BFwd>
        D_CONSTEXPR
        less_than_helper(
            _CFwd&& _comparator,
            _BFwd&& _bound
        )
            : m_comparator(std::forward<_CFwd>(_comparator))
            , m_bound(std::forward<_BFwd>(_bound))
        {}

        template<typename _V>
        D_CONSTEXPR
        bool operator()(
            const _V& _v
        ) const
        {
            return m_comparator(_v, m_bound);
        }

    private:
        _Comparator m_comparator;
        _Bound      m_bound;
    };


    template<typename _Comparator,
             typename _Bound>
    class greater_than_helper
    {
    public:
        template<typename _CFwd,
                 typename _BFwd>
        D_CONSTEXPR
        greater_than_helper(
            _CFwd&& _comparator,
            _BFwd&& _bound
        )
            : m_comparator(std::forward<_CFwd>(_comparator))
            , m_bound(std::forward<_BFwd>(_bound))
        {}

        template<typename _V>
        D_CONSTEXPR
        bool operator()(
            const _V& _v
        ) const
        {
            return m_comparator(m_bound, _v);
        }

    private:
        _Comparator m_comparator;
        _Bound      m_bound;
    };

NS_END  // internal


namespace comparators
{

    // equal_under
    //   function: derives an equality predicate from a
    // comparator. Returns a callable (a, b) -> bool that is
    // true iff a and b are equivalent under _comparator (i.e.
    // neither is less than the other).
    //
    //   Example: equal_under(by_key(&Person::dept))(p1, p2)
    template<typename _Comparator>
    D_NODISCARD D_CONSTEXPR internal::equal_under_helper<typename std::decay<_Comparator>::type>
    equal_under(
        _Comparator&& _comparator
    )
    {
        return internal::equal_under_helper<
            typename std::decay<_Comparator>::type>(
                std::forward<_Comparator>(_comparator));
    }


    // less_than
    //   function: builds a unary predicate by binding the second
    // argument of a comparator. less_than(cmp, x)(v) is true iff
    // cmp(v, x).
    template<typename _Comparator,
             typename _Bound>
    D_NODISCARD D_CONSTEXPR internal::less_than_helper<typename std::decay<_Comparator>::type,
                               typename std::decay<_Bound>::type>
    less_than(
        _Comparator&& _comparator,
        _Bound&&      _bound
    )
    {
        return internal::less_than_helper<
            typename std::decay<_Comparator>::type,
            typename std::decay<_Bound>::type>(
                std::forward<_Comparator>(_comparator),
                std::forward<_Bound>(_bound));
    }


    // greater_than
    //   function: builds a unary predicate by binding the first
    // argument of a comparator. greater_than(cmp, x)(v) is true
    // iff cmp(x, v) (i.e. v is greater than x under cmp).
    template<typename _Comparator,
             typename _Bound>
    D_NODISCARD D_CONSTEXPR internal::greater_than_helper<typename std::decay<_Comparator>::type,
                                  typename std::decay<_Bound>::type>
    greater_than(
        _Comparator&& _comparator,
        _Bound&&      _bound
    )
    {
        return internal::greater_than_helper<
            typename std::decay<_Comparator>::type,
            typename std::decay<_Bound>::type>(
                std::forward<_Comparator>(_comparator),
                std::forward<_Bound>(_bound));
    }

}   // namespace comparators


///////////////////////////////////////////////////////////////////////////////
///             VI.   STRUCTURAL TRAITS & CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////
//   Compile-time structural detection for the shapes this module produces
// and consumes. A comparator is a binary callable (a, b) -> bool; the derived
// predicates (less_than / greater_than) are unary callables v -> bool, and
// equal_under is a binary predicate. These traits answer "is this type
// callable in the comparator/predicate shape over the given operand types?"
// without requiring the callable to advertise any nested typedefs, so they
// recognise raw lambdas and std functors as readily as this module's own
// helpers.
//
//   The detection is expression-based and self-contained (each detector is a
// bespoke int/ellipsis overload pair, the same idiom already used by
// is_comparator_adapter_helper in section IV), so the traits introduce no
// dependency on a shared detection facility and cannot collide with one.

NS_INTERNAL

    // binary_call_detector
    //   helper: detects whether _Fn is callable as a const lvalue with two
    // const lvalue operands of type _A and _B and a result usable as a bool.
    template<typename _Fn,
             typename _A,
             typename _B>
    struct binary_call_detector
    {
    private:
        template<typename _F>
        static auto test(int) -> decltype(
            static_cast<bool>(
                std::declval<const _F&>()(std::declval<const _A&>(),
                                          std::declval<const _B&>())),
            std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        typedef decltype(test<_Fn>(0)) type;
        static D_CONSTEXPR bool value = type::value;
    };


    // unary_call_detector
    //   helper: detects whether _Fn is callable as a const lvalue with a
    // single const lvalue operand of type _V and a result usable as a bool.
    template<typename _Fn,
             typename _V>
    struct unary_call_detector
    {
    private:
        template<typename _F>
        static auto test(int) -> decltype(
            static_cast<bool>(
                std::declval<const _F&>()(std::declval<const _V&>())),
            std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        typedef decltype(test<_Fn>(0)) type;
        static D_CONSTEXPR bool value = type::value;
    };


    // result_type_detector
    //   helper: detects a nested result_type typedef. Present on the C++98
    // helpers and on the derived-predicate helpers; absent on the C++11
    // comparator helpers (which are transparently callable instead). Exposed
    // through has_result_type as a structural hint, not a requirement.
    template<typename _Type>
    struct result_type_detector
    {
    private:
        template<typename _T>
        static std::true_type  test(typename _T::result_type*);

        template<typename>
        static std::false_type test(...);

    public:
        typedef decltype(test<_Type>(0)) type;
        static D_CONSTEXPR bool value = type::value;
    };


    // strip
    //   helper: removes reference and cv-qualifiers so the traits below may
    // be queried on references and const types alike.
    template<typename _Type>
    struct strip
    {
        typedef typename std::remove_cv<
                    typename std::remove_reference<_Type>::type>::type type;
    };

NS_END  // internal


// is_binary_predicate
//   trait: true when _Fn(const _A&, const _B&) is well-formed and its result
// is usable as a bool. The general two-operand predicate shape; equal_under
// results and every comparator satisfy it.
template<typename _Fn,
         typename _A,
         typename _B>
struct is_binary_predicate
{
    static D_CONSTEXPR bool value =
        internal::binary_call_detector<typename internal::strip<_Fn>::type,
                                       _A, _B>::value;
};


// is_unary_predicate
//   trait: true when _Fn(const _V&) is well-formed and its result is usable
// as a bool. The shape produced by less_than / greater_than.
template<typename _Fn,
         typename _V>
struct is_unary_predicate
{
    static D_CONSTEXPR bool value =
        internal::unary_call_detector<typename internal::strip<_Fn>::type,
                                      _V>::value;
};


// is_comparator
//   trait: true when _Cmp is a comparator over _T, i.e. callable as
// _Cmp(const _T&, const _T&) with a bool-usable result. This is the central
// contract of the module: every factory (natural, by_key, by_member,
// by_function, lifted) and every combinator (reversed, then) yields a type
// that models is_comparator over its operand type.
template<typename _Cmp,
         typename _T>
struct is_comparator
{
    static D_CONSTEXPR bool value =
        internal::binary_call_detector<typename internal::strip<_Cmp>::type,
                                       _T, _T>::value;
};


// has_result_type
//   trait: true when _Type exposes a nested result_type typedef. A structural
// hint (present on C++98 helpers and derived-predicate helpers), not part of
// the callable contract.
template<typename _Type>
struct has_result_type
{
    static D_CONSTEXPR bool value =
        internal::result_type_detector<
            typename internal::strip<_Type>::type>::value;
};


// ---- convenience aliases ----
// Variable templates are a C++14 feature; gate the *_v shorthands so the
// header stays clean under -std=c++11 -pedantic. Pre-C++14 callers use the
// ::value form.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_comparator_v
//   constant: shorthand for is_comparator<_Cmp, _T>::value.
template<typename _Cmp,
         typename _T>
static D_CONSTEXPR bool is_comparator_v = is_comparator<_Cmp, _T>::value;

// is_binary_predicate_v
//   constant: shorthand for is_binary_predicate<_Fn, _A, _B>::value.
template<typename _Fn,
         typename _A,
         typename _B>
static D_CONSTEXPR bool is_binary_predicate_v =
    is_binary_predicate<_Fn, _A, _B>::value;

// is_unary_predicate_v
//   constant: shorthand for is_unary_predicate<_Fn, _V>::value.
template<typename _Fn,
         typename _V>
static D_CONSTEXPR bool is_unary_predicate_v =
    is_unary_predicate<_Fn, _V>::value;

// has_result_type_v
//   constant: shorthand for has_result_type<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool has_result_type_v = has_result_type<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// is_binary_predicate_c
//   concept: satisfied when _Fn is callable as a binary predicate over
// (_A, _B) with a bool-usable result. Concept parallel of
// is_binary_predicate, following the _c naming used in concepts.hpp.
template<typename _Fn,
         typename _A,
         typename _B>
concept is_binary_predicate_c = requires(
    const _Fn& _fn,
    const _A&  _a,
    const _B&  _b)
{
    static_cast<bool>(_fn(_a, _b));
};

// is_unary_predicate_c
//   concept: satisfied when _Fn is callable as a unary predicate over _V
// with a bool-usable result. Concept parallel of is_unary_predicate.
template<typename _Fn,
         typename _V>
concept is_unary_predicate_c = requires(
    const _Fn& _fn,
    const _V&  _v)
{
    static_cast<bool>(_fn(_v));
};

// is_comparator_c
//   concept: satisfied when _Cmp is a comparator over _T. Concept parallel
// of is_comparator and the natural way to constrain comparator-taking
// templates in C++20 call sites.
template<typename _Cmp,
         typename _T>
concept is_comparator_c = is_binary_predicate_c<_Cmp, _T, _T>;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp


#else  // !D_ENV_LANG_IS_CPP11_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
//   C++98 FALLBACK IMPLEMENTATION                                            //
///////////////////////////////////////////////////////////////////////////////


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    PRIMITIVE COMPARATOR HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // natural_helper
    //   helper: comparator that uses operator< on _T. The
    // default; equivalent to std::less<_T>.
    template<typename _T>
    struct natural_helper
    {
        typedef bool result_type;
        typedef _T   first_argument_type;
        typedef _T   second_argument_type;

        bool operator()(
            const _T& _a,
            const _T& _b
        ) const
        {
            return (_a < _b);
        }
    };


    // by_key_helper
    //   helper: extracts a key via _KeyFn and compares the
    // extracted keys with operator<. The key function is invoked
    // twice per comparison.
    template<typename _KeyFn>
    class by_key_helper
    {
    public:
        typedef bool result_type;

        explicit by_key_helper(
            const _KeyFn& _key_fn
        )
            : m_key_fn(_key_fn)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return (m_key_fn(_a) < m_key_fn(_b));
        }

    private:
        _KeyFn m_key_fn;
    };


    // by_function_helper
    //   helper: thin wrapper around an arbitrary binary callable
    // returning bool. Allows raw lambda-shaped functors to chain
    // with the combinators below.
    template<typename _Fn>
    class by_function_helper
    {
    public:
        typedef bool result_type;

        explicit by_function_helper(
            const _Fn& _fn
        )
            : m_fn(_fn)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_fn(_a, _b);
        }

    private:
        _Fn m_fn;
    };


    // member_accessor
    //   helper: callable that reads a data-member of _Class via
    // a stored pointer-to-member. Built by comparators::by_member
    // and fed into by_key_helper.
    template<typename _Class,
             typename _Member>
    class member_accessor
    {
    public:
        typedef _Member result_type;
        typedef _Class  argument_type;

        explicit member_accessor(
            _Member _Class::* _ptr
        )
            : m_ptr(_ptr)
        {}

        const _Member& operator()(
            const _Class& _obj
        ) const
        {
            return _obj.*m_ptr;
        }

    private:
        _Member _Class::* m_ptr;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   COMBINATOR HELPERS                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // reversed_helper
    //   helper: comparator that returns the inverse ordering of
    // _Inner by swapping argument order. Preserves strict weak
    // ordering even when _Inner is partial.
    template<typename _Inner>
    class reversed_helper
    {
    public:
        typedef bool result_type;

        explicit reversed_helper(
            const _Inner& _inner
        )
            : m_inner(_inner)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_inner(_b, _a);
        }

    private:
        _Inner m_inner;
    };


    // then_helper
    //   helper: tie-breaker chain. Compares with _Primary; on
    // equivalence (neither primary(a,b) nor primary(b,a)) defers
    // to _Secondary. Chains naturally: c1 | then(c2 | then(c3)).
    template<typename _Primary,
             typename _Secondary>
    class then_helper
    {
    public:
        typedef bool result_type;

        then_helper(
            const _Primary&   _primary,
            const _Secondary& _secondary
        )
            : m_primary(_primary)
            , m_secondary(_secondary)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            if (m_primary(_a, _b))
            {
                return true;
            }

            if (m_primary(_b, _a))
            {
                return false;
            }

            return m_secondary(_a, _b);
        }

    private:
        _Primary   m_primary;
        _Secondary m_secondary;
    };


    // lifted_helper
    //   helper: composes a comparator with a key function so the
    // result operates on a different type. _KeyFn is applied to
    // each argument; _Comparator runs on the extracted keys.
    template<typename _Comparator,
             typename _KeyFn>
    class lifted_helper
    {
    public:
        typedef bool result_type;

        lifted_helper(
            const _Comparator& _comparator,
            const _KeyFn&      _key_fn
        )
            : m_comparator(_comparator)
            , m_key_fn(_key_fn)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return m_comparator(m_key_fn(_a), m_key_fn(_b));
        }

    private:
        _Comparator m_comparator;
        _KeyFn      m_key_fn;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  PIPELINE ADAPTERS                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // then_adapter
    //   helper: RHS of operator|. Holds a secondary comparator;
    // when piped against a primary, yields a then_helper.
    template<typename _Secondary>
    class then_adapter
    {
    public:
        explicit then_adapter(
            const _Secondary& _secondary
        )
            : m_secondary(_secondary)
        {}

        template<typename _Primary>
        then_helper<_Primary, _Secondary>
        apply(
            const _Primary& _primary
        ) const
        {
            return then_helper<_Primary, _Secondary>(_primary,
                                                     m_secondary);
        }

    private:
        _Secondary m_secondary;
    };


    // reversed_adapter
    //   helper: RHS of operator| that wraps the LHS in a
    // reversed_helper. Lets callers write `cmp | reversed()` in
    // addition to `reversed(cmp)`.
    struct reversed_adapter
    {
        template<typename _Inner>
        reversed_helper<_Inner>
        apply(
            const _Inner& _inner
        ) const
        {
            return reversed_helper<_Inner>(_inner);
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             IV.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

namespace comparators
{

    // natural
    template<typename _T>
    internal::natural_helper<_T>
    natural()
    {
        return internal::natural_helper<_T>();
    }


    // by_key
    template<typename _KeyFn>
    internal::by_key_helper<_KeyFn>
    by_key(
        const _KeyFn& _key_fn
    )
    {
        return internal::by_key_helper<_KeyFn>(_key_fn);
    }


    // by_member
    //   function: builds a comparator that compares two objects
    // by a pointer-to-data-member. Reads more naturally at the
    // call site than spelling the equivalent by_key with a
    // hand-written accessor functor.
    template<typename _Class,
             typename _Member>
    internal::by_key_helper<internal::member_accessor<_Class, _Member> >
    by_member(
        _Member _Class::* _member_ptr
    )
    {
        return internal::by_key_helper<
            internal::member_accessor<_Class, _Member> >(
                internal::member_accessor<_Class, _Member>(_member_ptr));
    }


    // by_function
    //   function: builds a comparator from an arbitrary binary
    // callable returning bool.
    template<typename _Fn>
    internal::by_function_helper<_Fn>
    by_function(
        const _Fn& _fn
    )
    {
        return internal::by_function_helper<_Fn>(_fn);
    }


    // reversed
    //   function: wraps a comparator to produce the inverse
    // ordering.
    template<typename _Comparator>
    internal::reversed_helper<_Comparator>
    reversed(
        const _Comparator& _comparator
    )
    {
        return internal::reversed_helper<_Comparator>(_comparator);
    }


    // reversed (no-arg, adapter form)
    //   function: pipeline adapter; `cmp | reversed()` wraps cmp.
    inline
    internal::reversed_adapter
    reversed()
    {
        return internal::reversed_adapter();
    }


    // then
    //   function: builds a tie-breaker chain.
    template<typename _Primary,
             typename _Secondary>
    internal::then_helper<_Primary, _Secondary>
    then(
        const _Primary&   _primary,
        const _Secondary& _secondary
    )
    {
        return internal::then_helper<_Primary, _Secondary>(_primary,
                                                           _secondary);
    }


    // then (single-arg, adapter form)
    //   function: pipeline adapter; `c1 | then(c2)` yields a
    // then_helper(c1, c2).
    template<typename _Secondary>
    internal::then_adapter<_Secondary>
    then(
        const _Secondary& _secondary
    )
    {
        return internal::then_adapter<_Secondary>(_secondary);
    }


    // lifted
    //   function: composes a comparator with a key function.
    template<typename _Comparator,
             typename _KeyFn>
    internal::lifted_helper<_Comparator, _KeyFn>
    lifted(
        const _Comparator& _comparator,
        const _KeyFn&      _key_fn
    )
    {
        return internal::lifted_helper<_Comparator, _KeyFn>(_comparator,
                                                            _key_fn);
    }

}   // namespace comparators


///////////////////////////////////////////////////////////////////////////////
///             V.    PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// operator| (comparator | then_adapter)
//   function: forwards a comparator into a then_adapter to build
// a tie-breaker chain. Tight RHS typing prevents accidental
// firing on unrelated operands.
template<typename _Primary,
         typename _Secondary>
internal::then_helper<_Primary, _Secondary>
operator|(
    const _Primary&                            _primary,
    const internal::then_adapter<_Secondary>&  _adapter
)
{
    return _adapter.apply(_primary);
}


// operator| (comparator | reversed_adapter)
template<typename _Primary>
internal::reversed_helper<_Primary>
operator|(
    const _Primary&                   _primary,
    const internal::reversed_adapter& _adapter
)
{
    return _adapter.apply(_primary);
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   DERIVED PREDICATES                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // equal_under_helper
    //   helper: derives an equality predicate from a comparator
    // via strict-weak-ordering: a == b iff !cmp(a,b) && !cmp(b,a).
    template<typename _Comparator>
    class equal_under_helper
    {
    public:
        typedef bool result_type;

        explicit equal_under_helper(
            const _Comparator& _comparator
        )
            : m_comparator(_comparator)
        {}

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            return !m_comparator(_a, _b)
                && !m_comparator(_b, _a);
        }

    private:
        _Comparator m_comparator;
    };


    // less_than_helper
    //   helper: one-sided binder. less_than(cmp, x)(v) is cmp(v, x).
    template<typename _Comparator,
             typename _Bound>
    class less_than_helper
    {
    public:
        typedef bool result_type;

        less_than_helper(
            const _Comparator& _comparator,
            const _Bound&      _bound
        )
            : m_comparator(_comparator)
            , m_bound(_bound)
        {}

        template<typename _V>
        bool operator()(
            const _V& _v
        ) const
        {
            return m_comparator(_v, m_bound);
        }

    private:
        _Comparator m_comparator;
        _Bound      m_bound;
    };


    // greater_than_helper
    //   helper: one-sided binder. greater_than(cmp, x)(v) is
    // cmp(x, v).
    template<typename _Comparator,
             typename _Bound>
    class greater_than_helper
    {
    public:
        typedef bool result_type;

        greater_than_helper(
            const _Comparator& _comparator,
            const _Bound&      _bound
        )
            : m_comparator(_comparator)
            , m_bound(_bound)
        {}

        template<typename _V>
        bool operator()(
            const _V& _v
        ) const
        {
            return m_comparator(m_bound, _v);
        }

    private:
        _Comparator m_comparator;
        _Bound      m_bound;
    };

NS_END  // internal


namespace comparators
{

    // equal_under
    template<typename _Comparator>
    internal::equal_under_helper<_Comparator>
    equal_under(
        const _Comparator& _comparator
    )
    {
        return internal::equal_under_helper<_Comparator>(_comparator);
    }


    // less_than
    template<typename _Comparator,
             typename _Bound>
    internal::less_than_helper<_Comparator, _Bound>
    less_than(
        const _Comparator& _comparator,
        const _Bound&      _bound
    )
    {
        return internal::less_than_helper<_Comparator, _Bound>(_comparator,
                                                               _bound);
    }


    // greater_than
    template<typename _Comparator,
             typename _Bound>
    internal::greater_than_helper<_Comparator, _Bound>
    greater_than(
        const _Comparator& _comparator,
        const _Bound&      _bound
    )
    {
        return internal::greater_than_helper<_Comparator, _Bound>(_comparator,
                                                                  _bound);
    }

}   // namespace comparators


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_FUNCTIONAL_COMPARATOR_HPP_
