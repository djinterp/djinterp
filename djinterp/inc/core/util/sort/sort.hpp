/******************************************************************************
* djinterp [util]                                                     sort.hpp
*
* djinterp portable sorting algorithm framework
*   Provides core types, convenience aliases, and comparator infrastructure
* shared by all sorting algorithm headers.  Individual algorithm
* implementations (e.g. insertion_sort.hpp, merge_sort.hpp) include this
* header and expose their own named entry-point functions.
*
*   The header is designed to compile cleanly from C++98 through C++23 by
* gating modern features behind the project's D_ENV_* detection macros.
*
*
* path:      /inc/core/util/sort/sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_
#define DJINTERP_UTILITY_SORT_ 1

#include <stddef.h>
#include "../../djinterp.hpp"


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


#define D_KEYWORD_SORT  sort

#define NS_SORT         D_NAMESPACE(D_KEYWORD_SORT)


NS_SORT  // namespace sort

// sort_order
//   enum: specifies the ordering direction of a sort operation.
enum sort_order
{
    DSortOrderAscending  = 0,
    DSortOrderDescending = 1
};


// default comparators

// less
//   struct: default ascending comparator.  Returns true when _a precedes _b
// in strict weak ordering.  Mirrors std::less but is always available.
template<typename _Type>
struct less
{
    bool operator()(const _Type& _a,
                    const _Type& _b) const
    {
        return (_a < _b);
    }
};

// greater
//   struct: default descending comparator.  Returns true when _a follows _b
// in strict weak ordering.
template<typename _Type>
struct greater
{
    bool operator()(const _Type& _a,
                    const _Type& _b) const
    {
        return (_a > _b);
    }
};

// order_comparator - wraps a comparator with a sort-order flip

NS_INTERNAL

    // order_comparator
    //   struct: adapts a comparator to honour a runtime sort_order by
    // optionally reversing the comparison sense.
    template<typename _Compare>
    struct order_comparator
    {
    private:
        _Compare   m_comp;
        sort_order m_order;

    public:
        order_comparator(_Compare   _comp,
                         sort_order _order)
            : m_comp(_comp)
            , m_order(_order)
        {
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a,
                        const _B& _b) const
        {
            if (m_order == DSortOrderDescending)
            {
                return m_comp(_b, _a);
            }

            return m_comp(_a, _b);
        }
    };

NS_END  // internal

// comparator traits  (>C++11)

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // is_comparator_helper
    //   trait: SFINAE helper that detects whether _Comp(_a, _b) is
    // well-formed and yields a boolean-convertible result.
    template<typename _Comp,
             typename _Type,
             typename = void>
    struct is_comparator_helper
    {
        static const bool value = false;
    };

    // is_comparator_helper  (success case)
    //   trait: partial specialisation when _Comp is callable with two
    // const _Type& arguments and the result converts to bool.
    template<typename _Comp,
             typename _Type>
    struct is_comparator_helper
        <
            _Comp,
            _Type,
            typename std::enable_if
                <
                    std::is_convertible
                    <
                        decltype(
                            std::declval<_Comp>()(
                                std::declval<const _Type&>(),
                                std::declval<const _Type&>())),
                        bool
                    >::value
                >::type
        >
    {
        static const bool value = true;
    };

NS_END  // internal

// is_comparator
//   trait: determines whether _Comp models a strict-weak-ordering
// comparator over _Type.
template<typename _Comp,
         typename _Type>
struct is_comparator
{
    static const bool value =
        internal::is_comparator_helper<_Comp, _Type>::value;
};

// is_comparator_v
//   alias: convenience variable template (C++14+).
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Comp,
         typename _Type>
constexpr bool is_comparator_v = is_comparator<_Comp, _Type>::value;
#endif  // C++14

#endif  // C++11


// convenience aliases

// default_comparator
//   type: the default strict-weak-ordering comparator for a given
// element type.  Produces ascending order.
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


NS_END


#endif  // DJINTERP_UTILITY_SORT_
