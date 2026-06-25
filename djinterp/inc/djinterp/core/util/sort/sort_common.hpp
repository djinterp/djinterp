/******************************************************************************
* djinterp [utility]                                           sort_common.hpp
*
* djinterp portable sorting algorithm framework -- shared core.
*   Provides the core types, convenience aliases, and comparator infrastructure
* shared by every sorting algorithm header.  Individual algorithm
* implementations (insertion_sort.hpp, merge_sort.hpp, ...) include this header
* and expose their own named entry-point functions.
*   The header is designed to compile cleanly from C++98 through C++23 by gating
* modern features behind the project's D_ENV_* detection macros.
*
*   FUNCTIONAL INTEGRATION.  As of the functional-layer integration this header
* pulls in functional/comparator.hpp, so the sort subsystem speaks the same
* first-class comparator vocabulary as the rest of the framework: natural,
* by_key, by_member, by_function and lifted, composed through reversed and
* then.  Each of those models is_comparator<C, T> (the single, canonical
* definition now lives in comparator.hpp) and is a std::sort-convention binary
* callable, so it drops directly into every X_sort entry point and into the
* dispatch facility (sort_dispatch.hpp) with no glue:
*
*     djinterp::sort(v.begin(), v.end(),
*                    by_key(&person::age) | then(by_member(&person::name)));
*
*   The native less / greater comparators and the runtime order_comparator are
* retained as the subsystem's lightweight primitives (the algorithm headers
* build on them directly).  less<T> is the value-type equivalent of natural<T>()
* and greater<T> of natural<T>() | reversed(); new call sites are encouraged to
* prefer the comparator algebra, which composes.
*
*
* path:      /inc/djinterp/core/util/sort/sort_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_
#define DJINTERP_UTILITY_SORT_ 1

// std
#include <stddef.h>
#include <cstdint>
// djinterp
#include "../../djinterp.hpp"
#include "../../functional/comparator.hpp"   // first-class comparator algebra
                                             // (+ the canonical is_comparator)


#if D_ENV_CPP98_HAS_FUNCTIONAL
    #include <functional>
#endif

#if D_ENV_CPP98_HAS_ITERATOR
    #include <iterator>
#endif

#if D_ENV_CPP98_HAS_UTILITY
    #include <utility>
#endif

#if D_ENV_CPP98_HAS_ALGORITHM
    #include <algorithm>
#endif

// C++11: type_traits for SFINAE
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>
#endif


NS_DJINTERP

// sort_order
//   enum: specifies the ordering direction of a sort operation.  The
// compile-time equivalent of descending is reversed(comp) from the comparator
// algebra; sort_order carries the same choice when it must be made at run time.
enum class sort_order : std::uint8_t
{
    none,
    ascending,
    descending
};

// default comparators
// less
//   struct: default ascending comparator.  Returns true when _a precedes _b
// in strict weak ordering.  The value-type equivalent of natural<_Type>();
// retained because the algorithm headers default to it directly.
template<typename _Type>
struct less
{
    bool operator()(
        const _Type& _a,
        const _Type& _b
    ) const
    {
        return (_a < _b);
    }
};

// greater
//   struct: default descending comparator.  Returns true when _a follows _b
// in strict weak ordering.  The value-type equivalent of
// natural<_Type>() | reversed().
template<typename _Type>
struct greater
{
    bool operator()(
        const _Type& _a,
        const _Type& _b
    ) const
    {
        return (_a > _b);
    }
};

// order_comparator - wraps a comparator with a sort-order flip

NS_INTERNAL

    // order_comparator
    //   struct: adapts a comparator to honour a runtime sort_order by
    // optionally reversing the comparison sense.  This is the run-time
    // counterpart of the comparator algebra's reversed(): reversed wraps the
    // descending choice into the type at compile time, whereas order_comparator
    // selects it from a sort_order value the caller supplies at run time
    // (descending swaps the operands, exactly as reversed_helper does).
    template<typename _Compare>
    struct order_comparator
    {
    private:
        _Compare   m_comp;
        sort_order m_order;

    public:
        order_comparator(
            _Compare   _comp,
            sort_order _order
        )
            : m_comp(_comp),
              m_order(_order)
        {
        }

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a,
            const _B& _b
        ) const
        {
            if (m_order == sort_order::descending)
            {
                return m_comp(_b, _a);
            }

            return m_comp(_a, _b);
        }
    };

NS_END  // internal


//   comparator detection (is_comparator / is_comparator_v) is no longer defined
// here: the single canonical definition now lives in functional/comparator.hpp
// (included above), so the sort subsystem and the functional layer share one
// trait rather than two clashing primaries on the umbrella build.


// convenience aliases

// default_comparator
//   type: the default strict-weak-ordering comparator for a given element
// type.  Produces ascending order.
template<typename _Type>
struct default_comparator
{
    typedef less<_Type> type;
};

// For C++11+ we can use a proper alias template; for C++98 users
// must write default_comparator<T>::type.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// default_comparator_t
//   type: alias template shorthand.
template<typename _Type>
using default_comparator_t = typename default_comparator<_Type>::type;

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_
