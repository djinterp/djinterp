/******************************************************************************
* djinterp [functional]                                          extractor.hpp
*
* First-class extractors / projections (C++).
*   An extractor is a callable of signature `_Target(const _Source&)` that
* reads ("projects") some feature out of a value. Comparators, accumulators,
* and views all consume key functions; this module elevates those key
* functions to first-class values with their own combinators: composition,
* fan-out (produce a tuple), defaulting, predicate-gating, post-mapping,
* and try-extract (exceptions captured into maybe<T>).
*
*   Where accumulators absorb a stream into a value and producers spin a
* stream from nothing, an extractor reads a single value from a single
* value. It is the smallest unit in the dataflow vocabulary: a read-only
* "lens" over its source.
*
*   Every primitive helper carries D_CONSTEXPR, so an extractor built from
* constexpr callables is itself usable in a constant expression. The
* container-driving free functions at the bottom (extract_all,
* extract_into_map, group_by_extractor) are runtime-only because the
* containers they produce are not constexpr-allocatable in C++11; under
* C++20 with constexpr <vector> and <map> they become constexpr too,
* without source changes.
*
* USAGE:
*   struct person { std::string name; int age; std::string dept; };
*
*   auto e_age  = extractors::from_member(&person::age);
*   auto e_name = extractors::from_member(&person::name);
*
*   int n = e_age(some_person);
*
*   // composed: read p.name, then its length
*   auto e_name_len =
*       extractors::then_extract(e_name,
*                                [](const std::string& s) { return s.size(); });
*
*   // fan-out: a (age, dept) tuple per person
*   auto e_age_and_dept = extractors::fanout(e_age,
*                                            extractors::from_member(
*                                                &person::dept));
*
*   // safe extraction with predicate -> maybe<int>
*   auto e_adult_age = extractors::filtered(e_age,
*                                           [](int a) { return a >= 18; });
*
*   // pipeline form
*   auto pipeline_ext = extractors::from_member(&person::name)
*                     | extractors::mapped([](const std::string& s)
*                                          { return s.size(); });
*
*   // container drivers
*   auto names = extract_all(e_name, my_people);
*   auto by_dept = group_by_extractor(
*                      extractors::from_member(&person::dept),
*                      my_people);
*
*
* path:      /inc/djinterp/core/functional/extractor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL EXTRACTOR HELPER CLASSES
      1.  identity_helper                       (return source unchanged)
      2.  constant_helper                       (return stored value)
      3.  function_helper                       (wrap arbitrary callable)
      4.  member_helper                         (pointer-to-data-member)
      5.  index_helper                          (std::get<_N>)
      6.  composed_helper                       (e2(e1(x)))
      7.  fanout2_helper                        (tuple of two outputs)
      8.  fanout3_helper                        (tuple of three outputs)
      9.  mapped_helper                         (post-transform output)
      10. filtered_helper                       (predicate gate -> maybe)
      11. guarded_helper                        (source-side guard -> maybe)
      12. defaulted_helper                      (replace nothing with default)
      13. try_helper                            (catch -> maybe)
      14. then_extract_adapter                  (pipeline RHS)
      15. mapped_adapter                        (pipeline RHS)
      16. filtered_adapter                      (pipeline RHS)
II.   FACTORIES (namespace extractors)
      1.  identity<_Source>()
      2.  constant(value)
      3.  from_function(fn)
      4.  from_member(memptr)
      5.  from_index<_N>()
      6.  then_extract(inner, outer)            (also single-arg adapter form)
      7.  fanout(e1, e2) / fanout(e1, e2, e3)
      8.  mapped(e, fn)                         (also single-arg adapter form)
      9.  filtered(e, p)                        (also single-arg adapter form)
      10. guarded(e, guard_on_source)
      11. defaulted(maybe_extractor, default)
      12. try_extract(e)
III.  PIPELINE OPERATORS
      1.  operator|(extractor, then_extract_adapter)
      2.  operator|(extractor, mapped_adapter)
      3.  operator|(extractor, filtered_adapter)
IV.   CONTAINER DRIVERS  (runtime-only)
      1.  extract_all(e, container)             -> vector<_Target>
      2.  extract_first(e, container)           -> maybe<_Target>
      3.  extract_unique(e, container)          -> vector<_Target>
      4.  extract_into_map(key_e, value_e, c)   -> map<_Key, _Value>
      5.  group_by_extractor(e, container)      -> map<_Key, vector<_Source>>
V.    STRUCTURAL TRAITS & CONCEPTS
      1.  is_extractor<_Fn, _Source>            (unary, non-void result)
      2.  extractor_result_t<_Fn, _Source>      (decayed result type)
      3.  is_maybe<_Type>                        (is a maybe<T> specialization)
      4.  is_maybe_extractor<_Fn, _Source>      (extractor yielding maybe<T>)
      5.  *_v aliases  (C++14)                   (variable-template shorthands)
      6.  extractor_c / maybe_extractor_c (C++20) (concept parallels)
*/


#ifndef DJINTERP_FUNCTIONAL_EXTRACTOR_
#define DJINTERP_FUNCTIONAL_EXTRACTOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <map>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"
#include "./maybe.hpp"


NS_DJINTERP

// maybe / just / nothing are flat djinterp types (the functional sub-
// namespace has been retired; see the maybe / monad module). The
// unqualified references throughout this header resolve to them directly.


//   DUAL DOMAIN.  An extractor is a pure projection _Target(const _Source&),
// and every primitive helper here is D_CONSTEXPR, so an extractor built from
// constexpr callables folds during constant evaluation just as it runs at run
// time - over ordinary values, and over carrier leaves (val_t / type_t) when
// the projection reads the carrier (e.g. from_function([](auto c){ return
// c.value; })).  The container-driving free functions at the foot of the file
// (extract_all, extract_into_map, group_by_extractor) are runtime-only because
// they allocate; under C++20's constexpr <vector> / <map> they fold too, with
// no source change.  is_maybe / is_maybe_v are reused from maybe.hpp here, not
// redefined.

///////////////////////////////////////////////////////////////////////////////
///             I.    INTERNAL EXTRACTOR HELPER CLASSES                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // identity_helper
    //   helper: extractor that returns its argument unchanged.
    // Useful as a slot in higher-order constructions where one
    // of the projections must be the source itself (e.g. building
    // a (source, key) tuple via fanout).
    template<typename _Source>
    struct identity_helper
    {
        D_CONSTEXPR
        const _Source& operator()(
            const _Source& _value
        ) const
        {
            return _value;
        }
    };


    // constant_helper
    //   helper: extractor that ignores its source and returns a
    // stored value. The source type is irrelevant; the helper is
    // invocable with any argument.
    template<typename _Target>
    class constant_helper
    {
    public:
        template<typename _TargetFwd>
        explicit D_CONSTEXPR
        constant_helper(
            _TargetFwd&& _value
        )
            : m_value(std::forward<_TargetFwd>(_value))
        {}

        template<typename _Source>
        D_CONSTEXPR
        const _Target& operator()(
            const _Source&
        ) const
        {
            return m_value;
        }

    private:
        _Target m_value;
    };


    // function_helper
    //   helper: thin wrapper around an arbitrary unary callable.
    // Provided so every extractor factory yields the same helper-
    // style structure; useful when a user has an extractor-shaped
    // lambda and wants to chain it with the combinators below.
    template<typename _Fn>
    class function_helper
    {
    public:
        template<typename _FnFwd>
        explicit D_CONSTEXPR
        function_helper(
            _FnFwd&& _fn
        )
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> decltype(std::declval<const _Fn&>()(_value))
        {
            return m_fn(_value);
        }

    private:
        _Fn m_fn;
    };


    // member_helper
    //   helper: callable that reads a data-member of _Class via
    // a stored pointer-to-member. Equivalent in effect to the
    // comparator module's member_accessor; kept distinct because
    // the two callers want different invocability shapes (binary
    // for comparator, unary for extractor).
    template<typename _Class,
             typename _Member>
    class member_helper
    {
    public:
        explicit D_CONSTEXPR
        member_helper(
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


    // index_helper
    //   helper: extractor that returns std::get<_N>(source).
    // Works on std::tuple, std::pair, and std::array. Bounds
    // checking is delegated to std::get; out-of-range indices
    // produce ordinary compile-time errors.
    template<std::size_t _N>
    struct index_helper
    {
        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> decltype(std::get<_N>(_value))
        {
            return std::get<_N>(_value);
        }
    };


    // composed_helper
    //   helper: applies _Inner first, then feeds the result into
    // _Outer.  composed_helper(e1, e2)(x) = e2(e1(x)).
    template<typename _Inner,
             typename _Outer>
    class composed_helper
    {
    public:
        template<typename _InnerFwd,
                 typename _OuterFwd>
        D_CONSTEXPR
        composed_helper(
            _InnerFwd&& _inner,
            _OuterFwd&& _outer
        )
            : m_inner(std::forward<_InnerFwd>(_inner)),
              m_outer(std::forward<_OuterFwd>(_outer))
        {}

        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> decltype(
                std::declval<const _Outer&>()(
                    std::declval<const _Inner&>()(_value)))
        {
            return m_outer(m_inner(_value));
        }

    private:
        _Inner m_inner;
        _Outer m_outer;
    };


    // fanout2_helper
    //   helper: applies two extractors to the same source and
    // returns std::tuple<T1, T2>.
    template<typename _Extractor1,
             typename _Extractor2>
    class fanout2_helper
    {
    public:
        template<typename _E1Fwd,
                 typename _E2Fwd>
        D_CONSTEXPR
        fanout2_helper(
            _E1Fwd&& _e1,
            _E2Fwd&& _e2
        )
            : m_e1(std::forward<_E1Fwd>(_e1)),
              m_e2(std::forward<_E2Fwd>(_e2))
        {}

        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> std::tuple<
                   typename std::decay<
                       decltype(std::declval<const _Extractor1&>()(_value))
                   >::type,
                   typename std::decay<
                       decltype(std::declval<const _Extractor2&>()(_value))
                   >::type>
        {
            return std::make_tuple(m_e1(_value), m_e2(_value));
        }

    private:
        _Extractor1 m_e1;
        _Extractor2 m_e2;
    };


    // fanout3_helper
    //   helper: ternary fan-out. Wider arities can be obtained by
    // nesting fanout2 / fanout3 -- the resulting tuples will nest
    // accordingly. We provide explicit 2- and 3-arg shapes so that
    // the common cases don't pay the metaprogramming cost of a
    // fully variadic implementation.
    template<typename _Extractor1,
             typename _Extractor2,
             typename _Extractor3>
    class fanout3_helper
    {
    public:
        template<typename _E1Fwd,
                 typename _E2Fwd,
                 typename _E3Fwd>
        D_CONSTEXPR
        fanout3_helper(
            _E1Fwd&& _e1,
            _E2Fwd&& _e2,
            _E3Fwd&& _e3
        )
            : m_e1(std::forward<_E1Fwd>(_e1)),
              m_e2(std::forward<_E2Fwd>(_e2)),
              m_e3(std::forward<_E3Fwd>(_e3))
        {}

        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> std::tuple<
                   typename std::decay<
                       decltype(std::declval<const _Extractor1&>()(_value))
                   >::type,
                   typename std::decay<
                       decltype(std::declval<const _Extractor2&>()(_value))
                   >::type,
                   typename std::decay<
                       decltype(std::declval<const _Extractor3&>()(_value))
                   >::type>
        {
            return std::make_tuple(m_e1(_value),
                                   m_e2(_value),
                                   m_e3(_value));
        }

    private:
        _Extractor1 m_e1;
        _Extractor2 m_e2;
        _Extractor3 m_e3;
    };


    // mapped_helper
    //   helper: applies _MapFn to the extractor's output. The
    // operational semantics are identical to composed_helper; the
    // two are kept separate so that pipeline syntax `e | mapped(f)`
    // reads naturally when the second stage is a plain
    // transformation function rather than another full extractor.
    template<typename _Extractor,
             typename _MapFn>
    class mapped_helper
    {
    public:
        template<typename _EFwd,
                 typename _FFwd>
        D_CONSTEXPR
        mapped_helper(
            _EFwd&& _e,
            _FFwd&& _fn
        )
            : m_e(std::forward<_EFwd>(_e)),
              m_fn(std::forward<_FFwd>(_fn))
        {}

        template<typename _Source>
        D_CONSTEXPR auto
        operator()(
            const _Source& _value
        ) const
            -> decltype(
                std::declval<const _MapFn&>()(
                    std::declval<const _Extractor&>()(_value)))
        {
            return m_fn(m_e(_value));
        }

    private:
        _Extractor m_e;
        _MapFn     m_fn;
    };


    // filtered_helper
    //   helper: applies the inner extractor, then returns
    // just(value) when _predicate(value) is true, else nothing.
    // The result type is maybe<inner-output>.  The predicate
    // tests the EXTRACTED value, not the source; use
    // guarded_helper for source-side gating.
    template<typename _Extractor,
             typename _Predicate>
    class filtered_helper
    {
    public:
        template<typename _EFwd,
                 typename _PFwd>
        D_CONSTEXPR filtered_helper(
            _EFwd&& _e,
            _PFwd&& _pred
        )
            : m_e(std::forward<_EFwd>(_e)),
              m_pred(std::forward<_PFwd>(_pred))
        {}

        template<typename _Source>
        D_CONSTEXPR
        auto operator()(
            const _Source& _value
        ) const
            -> maybe<typename std::decay<
                   decltype(std::declval<const _Extractor&>()(_value))
               >::type>
        {
            using extracted_t = typename std::decay<
                decltype(m_e(_value))>::type;

            return m_pred(m_e(_value))
                 ? just(m_e(_value))
                 : nothing<extracted_t>();
        }

    private:
        _Extractor m_e;
        _Predicate m_pred;
    };


    // guarded_helper
    //   helper: applies _Guard to the SOURCE; on true, returns
    // just(extractor(source)); on false, returns nothing. Use
    // this when the guard is cheaper than the extractor or when
    // the extractor is only valid for sources passing the guard
    // (e.g. null-pointer checks before pointer dereference).
    template<typename _Extractor,
             typename _Guard>
    class guarded_helper
    {
    public:
        template<typename _EFwd,
                 typename _GFwd>
        D_CONSTEXPR guarded_helper(
            _EFwd&& _e,
            _GFwd&& _guard
        )
            : m_e(std::forward<_EFwd>(_e)),
              m_guard(std::forward<_GFwd>(_guard))
        {}

        template<typename _Source>
        D_CONSTEXPR auto 
        operator()(
            const _Source& _value
        ) const
            -> maybe<typename std::decay<
                   decltype(std::declval<const _Extractor&>()(_value))
               >::type>
        {
            using extracted_t = typename std::decay<
                decltype(m_e(_value))>::type;

            return m_guard(_value)
                 ? just(m_e(_value))
                 : nothing<extracted_t>();
        }

    private:
        _Extractor m_e;
        _Guard     m_guard;
    };


    // defaulted_helper
    //   helper: lifts a maybe-returning extractor back to a total
    // extractor by substituting a stored default whenever the
    // inner returns nothing. The inner MUST return a maybe<T> --
    // this is the dual of filtered/guarded/try_extract.
    template<typename _Extractor,
             typename _Default>
    class defaulted_helper
    {
    public:
        template<typename _EFwd,
                 typename _DFwd>
        D_CONSTEXPR
        defaulted_helper(
            _EFwd&& _e,
            _DFwd&& _default
        )
            : m_e(std::forward<_EFwd>(_e)),
              m_default(std::forward<_DFwd>(_default))
        {}

        template<typename _Source>
        D_CONSTEXPR
        _Default operator()(
            const _Source& _value
        ) const
        {
            return m_e(_value).value_or(m_default);
        }

    private:
        _Extractor m_e;
        _Default   m_default;
    };


    // try_helper
    //   helper: invokes the inner extractor inside a try/catch
    // and wraps the outcome as maybe<T>: just(...) on success,
    // nothing() on any exception. NOT constexpr because try/catch
    // is forbidden in constant evaluation pre-C++26; everything
    // else in this header remains usable in constexpr contexts.
    template<typename _Extractor>
    class try_helper
    {
    public:
        template<typename _EFwd>
        explicit
        try_helper(
            _EFwd&& _e
        )
            : m_e(std::forward<_EFwd>(_e))
        {}

        template<typename _Source>
        auto operator()(
            const _Source& _value
        ) const
            -> maybe<typename std::decay<
                   decltype(std::declval<const _Extractor&>()(_value))
               >::type>
        {
            using extracted_t = typename std::decay<
                decltype(m_e(_value))>::type;

            try
            {
                return just(m_e(_value));
            }
            catch (...)
            {
                return nothing<extracted_t>();
            }
        }

    private:
        _Extractor m_e;
    };


    ///////////////////////////////////////////////////////////////////////////
    ///         PIPELINE ADAPTERS                                           ///
    ///////////////////////////////////////////////////////////////////////////

    // then_extract_adapter
    //   helper: pipeline RHS that holds the outer extractor;
    // operator| composes (inner | then_extract_adapter(outer))
    // into a composed_helper.
    template<typename _Outer>
    class then_extract_adapter
    {
    public:
        template<typename _OuterFwd>
        explicit D_CONSTEXPR
        then_extract_adapter(
            _OuterFwd&& _outer
        )
            : m_outer(std::forward<_OuterFwd>(_outer))
        {}

        template<typename _Inner>
        D_CONSTEXPR
        composed_helper<typename std::decay<_Inner>::type, _Outer>
        apply(
            _Inner&& _inner
        ) const
        {
            return composed_helper<
                typename std::decay<_Inner>::type, _Outer>(
                    std::forward<_Inner>(_inner), m_outer);
        }

    private:
        _Outer m_outer;
    };


    // mapped_adapter
    //   helper: pipeline RHS for `e | mapped(f)`.
    template<typename _MapFn>
    class mapped_adapter
    {
    public:
        template<typename _FFwd>
        explicit D_CONSTEXPR
        mapped_adapter(
            _FFwd&& _fn
        )
            : m_fn(std::forward<_FFwd>(_fn))
        {}

        template<typename _Extractor>
        D_CONSTEXPR
        mapped_helper<typename std::decay<_Extractor>::type, _MapFn>
        apply(
            _Extractor&& _e
        ) const
        {
            return mapped_helper<
                typename std::decay<_Extractor>::type, _MapFn>(
                    std::forward<_Extractor>(_e), m_fn);
        }

    private:
        _MapFn m_fn;
    };


    // filtered_adapter
    //   helper: pipeline RHS for `e | filtered(p)`. Yields a
    // filtered_helper that returns maybe<T>.
    template<typename _Predicate>
    class filtered_adapter
    {
    public:
        template<typename _PFwd>
        explicit D_CONSTEXPR
        filtered_adapter(
            _PFwd&& _pred
        )
            : m_pred(std::forward<_PFwd>(_pred))
        {}

        template<typename _Extractor>
        D_CONSTEXPR
        filtered_helper<typename std::decay<_Extractor>::type, _Predicate>
        apply(
            _Extractor&& _e
        ) const
        {
            return filtered_helper<
                typename std::decay<_Extractor>::type, _Predicate>(
                    std::forward<_Extractor>(_e), m_pred);
        }

    private:
        _Predicate m_pred;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// identity
//   function: returns an extractor that yields its source
// unchanged. The dual of constant. Useful as a no-op slot in
// higher-order constructions (e.g. fanout(identity<T>(), e)
// for "the source itself plus a derived feature").
template<typename _Source>
D_NODISCARD D_CONSTEXPR internal::identity_helper<_Source>
identity()
{
    return internal::identity_helper<_Source>{};
}


// constant
//   function: returns an extractor that ignores its source
// and always yields the supplied value. The source type is
// erased; the same constant extractor can be applied to any
// input.
template<typename _Target>
D_NODISCARD D_CONSTEXPR internal::constant_helper<typename std::decay<_Target>::type>
constant(
    _Target&& _value
)
{
    return internal::constant_helper<typename std::decay<_Target>::type>(std::forward<_Target>(_value));
}


// from_function
//   function: lifts an arbitrary unary callable into an
// extractor so that it participates in the pipeline operators
// and combinators below. The callable is stored by value
// (decayed), preserving constexpr-ability.
template<typename _Fn>
D_NODISCARD D_CONSTEXPR internal::function_helper<typename std::decay<_Fn>::type>
from_function(
    _Fn&& _fn
)
{
    return internal::function_helper<typename std::decay<_Fn>::type>(std::forward<_Fn>(_fn));
}


// from_member
//   function: returns an extractor that reads the supplied
// pointer-to-data-member from any object of the owning class.
//
//   Example: extractors::from_member(&person::age)
template<typename _Class,
            typename _Member>
D_NODISCARD D_CONSTEXPR internal::member_helper<_Class, _Member>
from_member(
    _Member _Class::* _member_ptr
)
{
    return internal::member_helper<_Class, _Member>(_member_ptr);
}


// from_index
//   function: returns an extractor that reads the _N-th
// element of a std::tuple / std::pair / std::array via
// std::get<_N>. Index is supplied as a non-type template
// parameter so the result type can be computed at compile
// time.
template<std::size_t _N>
D_NODISCARD D_CONSTEXPR internal::index_helper<_N>
from_index()
{
    return internal::index_helper<_N>{};
}


// then_extract
//   function: composes two extractors so that the outer is
// applied to the result of the inner. Equivalent in effect
// to mapped, but the name reads more naturally when both
// stages are first-class extractors rather than a post-
// transformation function.
template<typename _Inner,
            typename _Outer>
D_NODISCARD D_CONSTEXPR internal::composed_helper<typename std::decay<_Inner>::type,
                            typename std::decay<_Outer>::type>
then_extract(
    _Inner&& _inner,
    _Outer&& _outer
)
{
    return internal::composed_helper<
        typename std::decay<_Inner>::type,
        typename std::decay<_Outer>::type>(
            std::forward<_Inner>(_inner),
            std::forward<_Outer>(_outer));
}


// then_extract (single-arg, adapter form)
//   function: builds a pipeline adapter so that
// `inner | then_extract(outer)` composes the two. Overload
// resolution picks this form when no inner is supplied.
template<typename _Outer>
D_NODISCARD D_CONSTEXPR internal::then_extract_adapter<typename std::decay<_Outer>::type>
then_extract(
    _Outer&& _outer
)
{
    return internal::then_extract_adapter<
        typename std::decay<_Outer>::type>(
            std::forward<_Outer>(_outer));
}


// fanout (binary)
//   function: builds an extractor that applies two extractors
// to the same source and yields a std::tuple<T1, T2>.
template<typename _E1,
            typename _E2>
D_NODISCARD D_CONSTEXPR internal::fanout2_helper<typename std::decay<_E1>::type,
                            typename std::decay<_E2>::type>
fanout(
    _E1&& _e1,
    _E2&& _e2
)
{
    return internal::fanout2_helper<
        typename std::decay<_E1>::type,
        typename std::decay<_E2>::type>(
            std::forward<_E1>(_e1),
            std::forward<_E2>(_e2));
}


// fanout (ternary)
//   function: three-way fan-out yielding
// std::tuple<T1, T2, T3>. Wider arities can be assembled by
// nesting; the tuples will nest accordingly.
template<typename _E1,
            typename _E2,
            typename _E3>
D_NODISCARD D_CONSTEXPR internal::fanout3_helper<typename std::decay<_E1>::type,
                            typename std::decay<_E2>::type,
                            typename std::decay<_E3>::type>
fanout(
    _E1&& _e1,
    _E2&& _e2,
    _E3&& _e3
)
{
    return internal::fanout3_helper<
        typename std::decay<_E1>::type,
        typename std::decay<_E2>::type,
        typename std::decay<_E3>::type>(
            std::forward<_E1>(_e1),
            std::forward<_E2>(_e2),
            std::forward<_E3>(_e3));
}


// mapped
//   function: returns an extractor that applies _fn to the
// output of _e. Same effect as then_extract; the name choice
// is stylistic. `mapped` reads better when the second stage
// is a plain lambda; `then_extract` reads better when both
// stages are first-class extractors.
template<typename _Extractor,
            typename _Fn>
D_NODISCARD D_CONSTEXPR internal::mapped_helper<typename std::decay<_Extractor>::type,
                        typename std::decay<_Fn>::type>
mapped(
    _Extractor&& _e,
    _Fn&&        _fn
)
{
    return internal::mapped_helper<
        typename std::decay<_Extractor>::type,
        typename std::decay<_Fn>::type>(
            std::forward<_Extractor>(_e),
            std::forward<_Fn>(_fn));
}


// mapped (single-arg, adapter form)
//   function: pipeline form for `e | mapped(f)`.
template<typename _Fn>
D_NODISCARD D_CONSTEXPR internal::mapped_adapter<typename std::decay<_Fn>::type>
mapped(
    _Fn&& _fn
)
{
    return internal::mapped_adapter<
        typename std::decay<_Fn>::type>(
            std::forward<_Fn>(_fn));
}


// filtered
//   function: turns a total extractor into a partial one
// gated by a predicate on the extracted value. The returned
// extractor produces maybe<T>; nothing is returned when the
// predicate is false.
template<typename _Extractor,
            typename _Predicate>
D_NODISCARD D_CONSTEXPR internal::filtered_helper<typename std::decay<_Extractor>::type,
                            typename std::decay<_Predicate>::type>
filtered(
    _Extractor&& _e,
    _Predicate&& _pred
)
{
    return internal::filtered_helper<
        typename std::decay<_Extractor>::type,
        typename std::decay<_Predicate>::type>(
            std::forward<_Extractor>(_e),
            std::forward<_Predicate>(_pred));
}


// filtered (single-arg, adapter form)
//   function: pipeline form for `e | filtered(p)`.
template<typename _Predicate>
D_NODISCARD D_CONSTEXPR internal::filtered_adapter<typename std::decay<_Predicate>::type>
filtered(
    _Predicate&& _pred
)
{
    return internal::filtered_adapter<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_pred));
}


// guarded
//   function: like filtered, but the predicate runs against
// the SOURCE before extraction rather than against the
// extracted value. Useful when the extractor itself is only
// safe to invoke on sources passing the guard.
template<typename _Extractor,
            typename _Guard>
D_NODISCARD D_CONSTEXPR internal::guarded_helper<typename std::decay<_Extractor>::type,
                            typename std::decay<_Guard>::type>
guarded(
    _Extractor&& _e,
    _Guard&&     _guard
)
{
    return internal::guarded_helper<
        typename std::decay<_Extractor>::type,
        typename std::decay<_Guard>::type>(
            std::forward<_Extractor>(_e),
            std::forward<_Guard>(_guard));
}


// defaulted
//   function: converts a maybe-returning extractor into a
// total extractor by substituting a stored default whenever
// the inner returns nothing. Inverse of filtered/guarded.
template<typename _Extractor,
            typename _Default>
D_NODISCARD D_CONSTEXPR internal::defaulted_helper<typename std::decay<_Extractor>::type,
                            typename std::decay<_Default>::type>
defaulted(
    _Extractor&& _e,
    _Default&&   _default
)
{
    return internal::defaulted_helper<
        typename std::decay<_Extractor>::type,
        typename std::decay<_Default>::type>(
            std::forward<_Extractor>(_e),
            std::forward<_Default>(_default));
}


// try_extract
//   function: wraps an extractor so that any exception thrown
// during extraction is captured as nothing. Not D_CONSTEXPR
// because exception handling is forbidden in constant
// evaluation pre-C++26.
template<typename _Extractor>
D_NODISCARD
internal::try_helper<typename std::decay<_Extractor>::type>
try_extract(
    _Extractor&& _e
)
{
    return internal::try_helper<
        typename std::decay<_Extractor>::type>(
            std::forward<_Extractor>(_e));
}

///////////////////////////////////////////////////////////////////////////////
///             III.  PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////
// The operator| overloads below are tightly constrained on the right-hand
//   side to the three named adapter types, so they cannot conflict with
//   the pipeline operators in view.hpp, comparator.hpp, monad.hpp, or any
//   other module: each module's adapter types are distinct.

// operator| (extractor | then_extract_adapter)
//   function: pipeline composition. Yields a composed_helper
// equivalent to then_extract(inner, outer).
template<typename _Inner,
         typename _Outer>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Inner&&                                _inner,
    internal::then_extract_adapter<_Outer>  _adapter
)
    -> decltype(_adapter.apply(std::forward<_Inner>(_inner)))
{
    return _adapter.apply(std::forward<_Inner>(_inner));
}


// operator| (extractor | mapped_adapter)
//   function: pipeline composition. Yields a mapped_helper.
template<typename _Extractor,
         typename _MapFn>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Extractor&&                        _e,
    internal::mapped_adapter<_MapFn>    _adapter
)
    -> decltype(_adapter.apply(std::forward<_Extractor>(_e)))
{
    return _adapter.apply(std::forward<_Extractor>(_e));
}


// operator| (extractor | filtered_adapter)
//   function: pipeline composition. Yields a filtered_helper
// which returns maybe<T>.
template<typename _Extractor,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR auto
operator|(
    _Extractor&&                            _e,
    internal::filtered_adapter<_Predicate>  _adapter
)
    -> decltype(_adapter.apply(std::forward<_Extractor>(_e)))
{
    return _adapter.apply(std::forward<_Extractor>(_e));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   CONTAINER DRIVERS  (runtime-only)                     ///
///////////////////////////////////////////////////////////////////////////////
// The functions below build std::vector / std::map results, so they are
//   inherently runtime in C++11/14/17.  Under C++20 with constexpr <vector>
//   and <map> they participate in constant evaluation as well, without
//   source changes here.  The extractor primitives in sections I-III above
//   remain available in constant expressions regardless of C++ version.

// extract_all
//   function: applies _e to every element of _container and
// collects the extracted values into a std::vector in container
// order.
template<typename _Extractor,
         typename _Container>
D_NODISCARD auto
extract_all(
    const _Extractor& _e,
    const _Container& _container
)
    -> std::vector<typename std::decay<
           decltype(_e(*std::begin(_container)))
       >::type>
{
    using value_t = typename std::decay<
        decltype(_e(*std::begin(_container)))>::type;

    std::vector<value_t> result;

    for (const auto& element : _container)
    {
        result.push_back(_e(element));
    }

    return result;
}


// extract_first
//   function: returns just(_e(first-element)) if _container has
// at least one element; otherwise nothing.
template<typename _Extractor,
         typename _Container>
D_NODISCARD auto
extract_first(
    const _Extractor& _e,
    const _Container& _container
)
    -> maybe<typename std::decay<
           decltype(_e(*std::begin(_container)))
       >::type>
{
    using value_t = typename std::decay<
        decltype(_e(*std::begin(_container)))>::type;

    auto it = std::begin(_container);

    if (it == std::end(_container))
    {
        return nothing<value_t>();
    }

    return just<value_t>(_e(*it));
}


// extract_unique
//   function: applies _e to every element and returns the
// distinct extracted values in first-seen order. Comparison
// uses operator==; for large containers, prefer
// extract_into_map / group_by_extractor.
template<typename _Extractor,
         typename _Container>
D_NODISCARD auto
extract_unique(
    const _Extractor& _e,
    const _Container& _container
)
    -> std::vector<typename std::decay<
           decltype(_e(*std::begin(_container)))
       >::type>
{
    using value_t = typename std::decay<
        decltype(_e(*std::begin(_container)))>::type;

    std::vector<value_t> result;

    for (const auto& element : _container)
    {
        value_t v     = _e(element);
        bool    found = false;

        for (const auto& existing : result)
        {
            if (existing == v)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            result.push_back(v);
        }
    }

    return result;
}


// extract_into_map
//   function: applies _key_e and _value_e to each source element
// and builds a std::map<_Key, _Value>. Later duplicates overwrite
// earlier entries.
template<typename _KeyExtractor,
         typename _ValueExtractor,
         typename _Container>
D_NODISCARD auto
extract_into_map(
    const _KeyExtractor&    _key_e,
    const _ValueExtractor&  _value_e,
    const _Container&       _container
)
    -> std::map<typename std::decay<
                    decltype(_key_e(*std::begin(_container)))
                >::type,
                typename std::decay<
                    decltype(_value_e(*std::begin(_container)))
                >::type>
{
    using key_t = typename std::decay<
        decltype(_key_e(*std::begin(_container)))>::type;
    using val_t = typename std::decay<
        decltype(_value_e(*std::begin(_container)))>::type;

    std::map<key_t, val_t> result;

    for (const auto& element : _container)
    {
        result[_key_e(element)] = _value_e(element);
    }

    return result;
}


// group_by_extractor
//   function: applies _e to each source element and groups the
// sources into buckets keyed by the extracted value. Returns
// std::map<_Key, std::vector<_Source>> with bucket order
// preserved within each value.
template<typename _Extractor,
         typename _Container>
D_NODISCARD auto 
group_by_extractor(
    const _Extractor& _e,
    const _Container& _container
)
    -> std::map<typename std::decay<
                    decltype(_e(*std::begin(_container)))
                >::type,
                std::vector<typename std::decay<
                    decltype(*std::begin(_container))
                >::type>>
{
    using key_t = typename std::decay<decltype(_e(*std::begin(_container)))>::type;
    using val_t = typename std::decay<decltype(*std::begin(_container))>::type;

    std::map<key_t, std::vector<val_t>> result;

    for (const auto& element : _container)
    {
        result[_e(element)].push_back(element);
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    STRUCTURAL TRAITS & CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////
//   Compile-time structural detection for the shape this module produces and
// consumes. An extractor is a unary callable _Target(const _Source&) reading
// a feature out of a source; the partial forms (filtered / guarded /
// try_extract) yield maybe<_Target>. These traits answer "is this type
// callable in the extractor shape over the given source?" without requiring
// the callable to advertise any nested typedefs, so they recognise raw
// lambdas and std functors as readily as this module's own helpers.
//
//   The detection is expression-based and self-contained (a bespoke
// int/ellipsis overload pair, the same idiom used by is_inspectable in
// function_traits.hpp and the comparator detectors), so the traits introduce
// no dependency on a shared detection facility and cannot collide with one.

NS_INTERNAL

    // no_result
    //   helper: sentinel yielded by the result detector when the callable is
    // not invocable on the source. Plays the role nonesuch plays in the
    // standard detection idiom.
    struct no_result
    {};


    // extract_result_detector
    //   helper: yields the raw result type of invoking _Fn as a const lvalue
    // on a const _Source& (reference / cv preserved), or no_result when that
    // call is ill-formed.
    template<typename _Fn,
             typename _Source>
    struct extract_result_detector
    {
    private:
        template<typename _F>
        static auto test(int) -> decltype(
            std::declval<const _F&>()(std::declval<const _Source&>()));

        template<typename>
        static no_result test(...);

    public:
        typedef decltype(test<_Fn>(0)) raw_type;
    };


    // strip
    //   helper: removes reference and cv-qualifiers so the traits below may
    // be queried on references and const types alike, and so result types
    // are reported in decayed form.
    template<typename _Type>
    struct strip
    {
        typedef typename std::remove_cv<
                    typename std::remove_reference<_Type>::type>::type type;
    };


    // (is_maybe detection is provided by maybe.hpp's djinterp::is_maybe, which
    // this header already includes; no local is_maybe_impl is defined here, to
    // avoid duplicating - and redefining - that trait.)

NS_END  // internal


// extractor_result_t
//   alias: the decayed result of applying _Fn to a const _Source&. Yields
// internal::no_result when _Fn is not callable on _Source (query is_extractor
// first if that case is possible).
template<typename _Fn,
         typename _Source>
using extractor_result_t = typename internal::strip<
    typename internal::extract_result_detector<
        typename internal::strip<_Fn>::type, _Source>::raw_type>::type;


// is_extractor
//   trait: true when _Fn is callable as (const _Source&) with a non-void
// result, i.e. it models the extractor contract _Target(const _Source&).
// Every factory (identity, constant, from_function, from_member, from_index)
// and every combinator (then_extract, fanout, mapped, filtered, guarded,
// defaulted, try_extract) yields a type that models is_extractor over its
// source.
template<typename _Fn,
         typename _Source>
struct is_extractor
{
private:
    typedef typename internal::extract_result_detector<
        typename internal::strip<_Fn>::type, _Source>::raw_type raw_t;

public:
    static D_CONSTEXPR bool value =
        ( !std::is_same<raw_t, internal::no_result>::value &&
          !std::is_void<raw_t>::value );
};


// is_maybe
//   trait: reused from maybe.hpp (djinterp::is_maybe) - true when _Type is a
// maybe<_T> specialization after cv/ref are stripped.  This header includes
// maybe.hpp, so the trait is taken from there rather than redefined (reuse,
// not recreate); is_maybe_extractor and the is_maybe_v shorthand below resolve
// to that one definition.


// is_maybe_extractor
//   trait: true when _Fn is an extractor over _Source whose result is a
// maybe<_T>. The partial / safe extractors (filtered, guarded, try_extract)
// model this; the total extractors do not.
template<typename _Fn,
         typename _Source>
struct is_maybe_extractor
{
    static D_CONSTEXPR bool value =
        ( is_extractor<_Fn, _Source>::value &&
          is_maybe<extractor_result_t<_Fn, _Source> >::value );
};


// ---- convenience aliases ----
// Variable templates are a C++14 feature; gate the *_v shorthands so the
// header stays clean under -std=c++11 -pedantic. Pre-C++14 callers use the
// ::value form.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_extractor_v
//   constant: shorthand for is_extractor<_Fn, _Source>::value.
template<typename _Fn,
         typename _Source>
static D_CONSTEXPR bool is_extractor_v = is_extractor<_Fn, _Source>::value;

// is_maybe_v
//   constant: reused from maybe.hpp (djinterp::is_maybe_v); not redefined here
// (reuse, not recreate), consistent with is_maybe above.

// is_maybe_extractor_v
//   constant: shorthand for is_maybe_extractor<_Fn, _Source>::value.
template<typename _Fn,
         typename _Source>
static D_CONSTEXPR bool is_maybe_extractor_v =
    is_maybe_extractor<_Fn, _Source>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// extractor_c
//   concept: satisfied when _Fn is an extractor over _Source — callable as
// (const _Source&) with a non-void result. Concept parallel of is_extractor,
// following the _c naming used in concepts.hpp.
template<typename _Fn,
         typename _Source>
concept extractor_c =
    requires(const _Fn& _fn, const _Source& _src)
    {
        _fn(_src);
    }
    && ( !std::is_void_v<extractor_result_t<_Fn, _Source> > );

// maybe_extractor_c
//   concept: satisfied when _Fn is an extractor over _Source whose result is
// a maybe<_T>. Concept parallel of is_maybe_extractor.
template<typename _Fn,
         typename _Source>
concept maybe_extractor_c =
    extractor_c<_Fn, _Source>
    && is_maybe<extractor_result_t<_Fn, _Source> >::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_EXTRACTOR_