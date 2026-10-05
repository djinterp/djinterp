/*******************************************************************************
* djinterp [re_std]                                          shared_ptr_swap.hpp
*
* shared_ptr_swap swap specialization header:
* non-member ADL swap overload for shared_ptr. Delegates to the
* member swap. Mirrors any_swap.hpp / unique_ptr_swap.hpp.
*
*
* path:      /inc/re_std/memory/shared_ptr_swap.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_SHARED_PTR_SWAP_HPP
#define RE_STD_MEMORY_SHARED_PTR_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"


namespace re_std
{

template<typename T>
RE_STD_INLINE void swap(shared_ptr<T>& _lhs, shared_ptr<T>& _rhs) RE_STD_NOEXCEPT
{
    _lhs.swap(_rhs);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_SHARED_PTR_SWAP_HPP
