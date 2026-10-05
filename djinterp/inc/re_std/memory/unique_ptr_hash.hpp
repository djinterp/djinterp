/*******************************************************************************
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
* unique_ptr<Type, Deleter>::pointer rather than Type*.
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
* path:      /inc/re_std/memory/unique_ptr_hash.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNIQUE_PTR_HASH_HPP
#define RE_STD_MEMORY_UNIQUE_PTR_HASH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./unique_ptr.hpp"
#include "../functional/hash.hpp"


namespace re_std
{


// ===========================================================================
// I.   HASH<UNIQUE_PTR>
// ===========================================================================

// hash<unique_ptr<Type, Deleter>>
//   class: forwards to hash on the deleter-determined pointer type.
template<typename Type,
         typename Deleter>
struct hash< unique_ptr<Type, Deleter> >
{
    std::size_t
    operator()(
        const unique_ptr<Type, Deleter>& _p
    ) const
    {
        typedef typename unique_ptr<Type, Deleter>::pointer _Ptr;
        return hash<_Ptr>()(_p.get());
    }
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_MEMORY_UNIQUE_PTR_HASH_HPP
