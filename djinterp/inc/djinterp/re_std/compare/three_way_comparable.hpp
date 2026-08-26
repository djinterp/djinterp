/******************************************************************************
* djinterp [re_std]                                    three_way_comparable.hpp
*
* the three_way_comparable concept:
*   Constrains a type to having a usable operator<=> whose result category
* is at least as strong as the one asked for:
*
*       template<three_way_comparable<strong_ordering> T> void f(T);
*
*   THE CATEGORY PARAMETER IS AN UPPER BOUND ON WEAKNESS, NOT AN EQUALITY:
*   three_way_comparable<T, weak_ordering> is satisfied by a type whose
* <=> yields strong_ordering, because a strong ordering IS a weak one.
* It is NOT satisfied by one yielding partial_ordering. That direction is
* the whole point -- a caller asking for weak_ordering is saying "I need
* at least this much", and the check is written through
* common_comparison_category, which computes the weakest of the two and
* compares it to the requested category.
*
*   The default is partial_ordering, the weakest, so a bare
* three_way_comparable<T> asks only that <=> exist and be usable.
*
*   C++20 ONLY -- NO BACK-PORT IS POSSIBLE:
*   Both `concept` and `operator<=>` are language features. There is no
* C++11 spelling of either, so this header gates itself out below C++20
* rather than shipping a trait-shaped approximation that would accept
* different types. The coverage entry previously recorded a C++11 floor
* for this symbol, which was doubly wrong: the file did not exist, and
* the tier was unreachable in principle.
*
*   THE TWO EXPOSITION-ONLY HELPERS ARE REPRODUCED IN internal::
*   [cmp.concept] defines this in terms of __weakly-equality-comparable-with
* and __partially-ordered-with, neither of which the standard exposes.
* They are written out below rather than approximated with
* equality_comparable_with, which is a STRONGER requirement (it demands a
* common reference type) and would reject types the standard accepts.
*
*
* path:      /inc/djinterp/re_std/compare/three_way_comparable.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_
#define DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
//   required for the ordering types the builtin <=> yields.
#include <compare>

// re_std
#include "../concepts/same_as.hpp"
#include "../concepts/boolean_testable.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD

NS_INTERNAL

    // compares_as
    //   concept: _Comp is a comparison category at least as strong as
    // _Cat. Folds the two through common_comparison_category -- which
    // yields the WEAKER of them -- and asks whether the result is still
    // _Cat. If _Comp were weaker, the fold would produce _Comp and the
    // same_as check would fail.
    //
    //   THE CATEGORIES HERE ARE std::, NOT re_std::, AND THEY MUST BE:
    //   re_std::partial_ordering is re_std's OWN class, back-ported to
    // C++11. The builtin operator<=> is a LANGUAGE feature and yields
    // std::strong_ordering -- the language will never produce an re_std
    // category, so a concept whose entire subject is `decltype(a <=> b)`
    // has to speak std's vocabulary or match nothing at all. That was the
    // first version of this file: it folded a std category against an
    // re_std one, got void, and rejected `int`.
    //
    //   This is consistent with the rest of the module rather than a
    // departure from it -- compare_three_way_result already documents its
    // result as std::strong_ordering, for the same reason.
    //
    //   The consequence to know: pass std::weak_ordering as the category
    // argument, not re_std::weak_ordering. re_std's own category types
    // remain the right choice for a hand-written operator<=> return type
    // on a C++11-compatible class; they are simply not what the builtin
    // operator deals in.
    template<typename _Comp, typename _Cat>
    concept compares_as
        = same_as<::std::common_comparison_category_t<_Comp, _Cat>, _Cat>;

    // weakly_equality_comparable_with
    //   concept: __weakly-equality-comparable-with from [concept.equality
    // comparable], which the standard leaves exposition-only. Requires all
    // four mixed == and != forms, each usable as a condition. Deliberately
    // NOT equality_comparable_with, which additionally demands a common
    // reference type and would reject types the standard admits here.
    template<typename _T, typename _U>
    concept weakly_equality_comparable_with
        = requires(const typename remove_reference<_T>::type& _t,
                   const typename remove_reference<_U>::type& _u)
          {
              { _t == _u } -> boolean_testable;
              { _t != _u } -> boolean_testable;
              { _u == _t } -> boolean_testable;
              { _u != _t } -> boolean_testable;
          };

    // partially_ordered_with
    //   concept: __partially-ordered-with from [cmp.concept], also
    // exposition-only. All eight mixed relational forms.
    template<typename _T, typename _U>
    concept partially_ordered_with
        = requires(const typename remove_reference<_T>::type& _t,
                   const typename remove_reference<_U>::type& _u)
          {
              { _t <  _u } -> boolean_testable;
              { _t >  _u } -> boolean_testable;
              { _t <= _u } -> boolean_testable;
              { _t >= _u } -> boolean_testable;
              { _u <  _t } -> boolean_testable;
              { _u >  _t } -> boolean_testable;
              { _u <= _t } -> boolean_testable;
              { _u >= _t } -> boolean_testable;
          };

NS_END  // internal


    // three_way_comparable
    //   concept: _Type has a usable operator<=> whose category is at least
    // as strong as _Cat, and the ==, != and relational operators implied
    // by that ordering.
    template<typename _Type,
             typename _Cat = ::std::partial_ordering>
    concept three_way_comparable
        =  internal::weakly_equality_comparable_with<_Type, _Type>
        && internal::partially_ordered_with<_Type, _Type>
        && requires(const typename remove_reference<_Type>::type& _a,
                    const typename remove_reference<_Type>::type& _b)
           {
               { _a <=> _b } -> internal::compares_as<_Cat>;
           };

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_
