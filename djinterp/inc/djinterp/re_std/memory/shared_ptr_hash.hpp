/******************************************************************************
* djinterp [re_std]                                          shared_ptr_hash.hpp
*
* shared_ptr hash support header:
*   re_std::hash specialisation for shared_ptr.
*
*   THE CONTRACT, WHICH IS EASY TO GET WRONG:
*   hash<shared_ptr<T>>(p) is defined to equal hash<T*>(p.get()) -- the
* STORED POINTER, not the control block. Two shared_ptrs that own the
* same object through different control blocks therefore hash equally,
* and an aliasing shared_ptr hashes as its aliased pointer rather than
* as its owner. This is what keeps hash consistent with
* operator==(shared_ptr, shared_ptr), which also compares get().
*
*   A null shared_ptr hashes as the null pointer, which is well-defined
* and equal for every null shared_ptr of the same type.
*
*   PORTABILITY:
*   std added this in C++11; re_std matches. Not noexcept-annotated
* beyond what hash<T*> provides, and not constexpr -- hash<T*> uses a
* reinterpret_cast, which is non-constexpr on every tier.
*
*
* path:      /inc/djinterp/re_std/memory/shared_ptr_hash.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_SHARED_PTR_HASH_
#define DJINTERP_RE_STD_MEMORY_SHARED_PTR_HASH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./shared_ptr.hpp"
#include "../functional/hash.hpp"


NS_RESTD


// ===========================================================================
// I.   HASH<SHARED_PTR>
// ===========================================================================

// hash<shared_ptr<_Type>>
//   class: forwards to hash<element_type*> on the stored pointer, per
// [util.smartptr.hash].
template<typename _Type>
struct hash< shared_ptr<_Type> >
{
    std::size_t
    operator()(
        const shared_ptr<_Type>& _p
    ) const
    {
        typedef typename shared_ptr<_Type>::element_type _Elem;
        return hash<_Elem*>()(_p.get());
    }
};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_MEMORY_SHARED_PTR_HASH_
