/*******************************************************************************
* djinterp [core]                                                sort_common.hpp
*
* The ordering vocabulary the C++ sort algorithms share: the direction a sort
* arranges its range in, and the adapter that folds that direction into a
* comparator.
*   This is the C++ counterpart of the C module's c/util/sort/sort_common.h.
* There the direction is passed to every call; here bubble_sort_ordered,
* insertion_sort_ordered, merge_sort_ordered and selection_sort_ordered fold it
* into the comparator through internal::order_comparator, so the algorithms
* themselves see one strict-weak-ordering predicate and never branch on the
* direction.
*   sort_order keeps the C enum's values, so a direction crosses the C API as
* the same integer. It is scoped inside a struct at every language level, as
* decision 3.4 has it for C++98-floor modules: `sort_order::descending` spells
* the same at C++98 and C++23, and the type is `sort_order::value`.
*   Reconstructed on 2026.09.30 from its users: the four sort headers named
* both entities but nothing defined them, so none of the four could compile.
*
*
* path:      /inc/djinterp/core/util/sort/sort_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_SORT_COMMON_HPP
#define DJINTERP_UTIL_SORT_SORT_COMMON_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root


NS_DJINTERP

// sort_order
//   enum: the direction a sort arranges its range in, with the values of the
// C module's d_sort_order. `none` is treated as ascending everywhere; it
// exists so that a value-initialised request is usable rather than
// meaningless.
struct sort_order
{
    enum value
    {
        none       = 0,
        ascending  = 1,
        descending = 2
    };
};

NS_INTERNAL

    // order_comparator
    //   class: a std::sort-convention binary predicate that applies Comparator
    // in the direction a sort_order asks for. Descending exchanges the
    // operands, which turns the strict weak ordering "left precedes right"
    // into its converse, itself a strict weak ordering; ascending and none
    // pass them through unchanged. Called as a non-const lvalue, as the sort
    // algorithms hold their comparator by value, so a Comparator whose call
    // operator is not const works too.
    template<typename Comparator>
    class order_comparator
    {
    public:
        order_comparator(
            Comparator        _comparator,
            sort_order::value _order
        )
            : m_comparator(_comparator),
              m_descending(_order == sort_order::descending)
        {}

        template<typename Type>
        bool
        operator()(
            const Type& _left,
            const Type& _right
        )
        {
            if (m_descending)
            {
                return m_comparator(_right,
                                    _left);
            }

            return m_comparator(_left,
                                _right);
        }

    private:
        Comparator m_comparator;
        bool       m_descending;
    };

NS_END  // internal

NS_END  // djinterp


#endif  // DJINTERP_UTIL_SORT_SORT_COMMON_HPP
