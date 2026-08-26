/***********************************************************************
* re_std                                                 weak_ptr_swap.hpp
*
* non-member ADL swap overload for weak_ptr. Delegates to the
* member swap.
*
*
* path:      /inc/djinterp/re_std/memory/weak_ptr_swap.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_WEAK_PTR_SWAP_
#define DJINTERP_RE_STD_MEMORY_WEAK_PTR_SWAP_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/weak_ptr.hpp"


namespace re_std
{

template<typename _T>
D_INLINE void swap(weak_ptr<_T>& _lhs, weak_ptr<_T>& _rhs) D_NOEXCEPT
{
    _lhs.swap(_rhs);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_WEAK_PTR_SWAP_
