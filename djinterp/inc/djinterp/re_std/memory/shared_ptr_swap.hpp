/***********************************************************************
* restd                                                shared_ptr_swap.hpp
*
* non-member ADL swap overload for shared_ptr. Delegates to the
* member swap. Mirrors any_swap.hpp / unique_ptr_swap.hpp.
*
*
* path:      /inc/djinterp/re_std/memory/shared_ptr_swap.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_SHARED_PTR_SWAP_
#define RESTD_MEMORY_SHARED_PTR_SWAP_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"


namespace restd
{

template<typename _T>
D_INLINE void swap(shared_ptr<_T>& _lhs, shared_ptr<_T>& _rhs) D_NOEXCEPT
{
    _lhs.swap(_rhs);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_SHARED_PTR_SWAP_
