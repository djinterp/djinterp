/*******************************************************************************
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
* path:      /inc/re_std/memory/shared_ptr_hash.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_SHARED_PTR_HASH_HPP
#define RE_STD_MEMORY_SHARED_PTR_HASH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./shared_ptr.hpp"
#include "../functional/hash.hpp"


namespace re_std
{


// ===========================================================================
// I.   HASH<SHARED_PTR>
// ===========================================================================

// hash<shared_ptr<Type>>
//   class: forwards to hash<element_type*> on the stored pointer, per
// [util.smartptr.hash].
template<typename Type>
struct hash< shared_ptr<Type> >
{
    std::size_t
    operator()(
        const shared_ptr<Type>& _p
    ) const
    {
        typedef typename shared_ptr<Type>::element_type _Elem;
        return hash<_Elem*>()(_p.get());
    }
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_MEMORY_SHARED_PTR_HASH_HPP
