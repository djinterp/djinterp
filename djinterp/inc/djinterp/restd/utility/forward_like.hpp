/***********************************************************************
* restd                                                   forward_like.hpp
*
* forward_like<T>(_v):
*   Casts _v to the value category and const-qualification implied
* by T. Used in "deducing this" methods (and lambdas with explicit
* object parameters) to propagate the receiver's qualifiers when
* returning members:
*
*     template<class Self> auto&& get(this Self&& self) {
*         return restd::forward_like<Self>(self.member);
*     }
*
*   The propagation rules per [forward]:
*     - the const-ness of T transfers (T is const-qualified -> result
*       is const-qualified)
*     - the value category of T transfers:
*         T is an lvalue ref      -> result is an lvalue ref
*         T is anything else      -> result is an rvalue ref
*
*   So forward_like<const int&>(x) yields const int&.
*   forward_like<int>(x) yields int&&.
*
* added in std C++23.
*
*
* path:      /inc/restd/utility/forward_like.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_FORWARD_LIKE_
#define RESTD_UTILITY_FORWARD_LIKE_ 1

#include "djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    #include "../type_traits/remove_reference.hpp"

    // is_lvalue_reference + is_const are needed for the type
    // selection. Documented dependency: these two restd traits
    // were shipped in the type_traits foundation phase.
    #include "../type_traits/is_lvalue_reference.hpp"
    #include "../type_traits/is_const.hpp"


namespace restd
{
namespace internal
{

    // Helper: const-add iff Cond.
    template<bool _Cond, typename _U> struct cond_const;
    template<typename _U> struct cond_const<true, _U>
    {
        typedef const _U type;
    };
    template<typename _U> struct cond_const<false, _U>
    {
        typedef _U type;
    };

    // Helper: pick & or && based on a bool.
    template<bool _IsLvalue, typename _U> struct cond_ref;
    template<typename _U> struct cond_ref<true, _U>
    {
        typedef _U& type;
    };
    template<typename _U> struct cond_ref<false, _U>
    {
        typedef _U&& type;
    };

    // Compute the result type of forward_like<T> applied to a
    // value of type U. Two axes: lvalue-ness (from T) and const-
    // ness (from T's referent).
    template<typename _T, typename _U>
    struct forward_like_result
    {
    private:
        // Strip the reference from T to inspect const.
        typedef typename remove_reference<_T>::type _TBare;

        // Strip from U as well.
        typedef typename remove_reference<_U>::type _UBare;

        // Combine: if T's referent is const, U is also const.
        typedef typename cond_const<is_const<_TBare>::value,
                                    _UBare>::type _MaybeConst;

    public:
        // Pick && or & based on T's value category.
        typedef typename cond_ref<is_lvalue_reference<_T>::value,
                                  _MaybeConst>::type type;
    };

}  // namespace internal


template<typename _T, typename _U>
D_CONSTEXPR
typename internal::forward_like_result<_T, _U>::type
forward_like(_U&& _v) D_NOEXCEPT
{
    typedef typename internal::forward_like_result<_T, _U>::type _R;
    return static_cast<_R>(_v);
}


}  // namespace restd

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_FORWARD_LIKE_
