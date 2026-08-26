/******************************************************************************
* djinterp [re_std]                                          unique_ptr_hash.hpp
*
* unique_ptr hash support header:
*   re_std::hash specialisation for unique_ptr.
*
*   hash<unique_ptr<T, D>>(p) equals hash<unique_ptr<T,D>::pointer>
* applied to p.get(). Note the key type: it is the DELETER'S pointer
* type, not T*. A deleter that defines a nested `pointer` typedef (a
* fancy pointer, an offset handle) changes what unique_ptr stores, and
* the hash has to follow it -- which is why this specialisation names
* unique_ptr<_Type, _Deleter>::pointer rather than _Type*.
*
*   In std this specialisation is CONDITIONALLY enabled -- it exists
* only when hash<pointer> is itself enabled. re_std's hash primary
* template is empty rather than deleted, so an unusable pointer type
* already fails at the point of instantiation with a clear error, and
* the extra SFINAE layer would buy nothing here.
*
*   PORTABILITY:
*   std added this in C++11; re_std matches.
*
*
* path:      /inc/djinterp/re_std/memory/unique_ptr_hash.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_HASH_
#define DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_HASH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./unique_ptr.hpp"
#include "../functional/hash.hpp"


NS_RESTD


// ===========================================================================
// I.   HASH<UNIQUE_PTR>
// ===========================================================================

// hash<unique_ptr<_Type, _Deleter>>
//   class: forwards to hash on the deleter-determined pointer type.
template<typename _Type,
         typename _Deleter>
struct hash< unique_ptr<_Type, _Deleter> >
{
    std::size_t
    operator()(
        const unique_ptr<_Type, _Deleter>& _p
    ) const
    {
        typedef typename unique_ptr<_Type, _Deleter>::pointer _Ptr;
        return hash<_Ptr>()(_p.get());
    }
};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_HASH_
