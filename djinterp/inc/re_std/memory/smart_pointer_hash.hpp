/******************************************************************************
* re_std [memory]                                        smart_pointer_hash.hpp
*
*   hash<shared_ptr<T>> and hash<unique_ptr<T, D>>.
*
*   BOTH HASH THE STORED POINTER, NOT THE POINTEE, and that is the only
* defensible choice: hashing the pointee would disagree with operator==, which
* compares pointers. Two shared_ptrs to equal-but-distinct objects are NOT
* equal and must not hash equally; two shared_ptrs to the SAME object are
* equal and do.
*
*   hash<shared_ptr> USES get(), NOT owner_before. That means two shared_ptrs
* that share ownership but hold different stored pointers - an aliasing
* constructor's product, say - hash differently, which again matches
* operator== rather than owner ordering. If you want owner identity, that is
* what owner_less and owner_hash are for.
*
*   unique_ptr's specialisation is enabled only when hash<pointer> is - std
* requires the disabled case to be a disabled specialisation rather than a
* hard error, so generic code can query it.
*
*   STD IS C++11; re_std IS C++11 - inherits the smart pointers' own floor.
*
* path:      /inc/djinterp/re_std/memory/smart_pointer_hash.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_SMART_POINTER_HASH_
#define DJINTERP_RE_STD_MEMORY_SMART_POINTER_HASH_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./shared_ptr.hpp"
#include "./unique_ptr.hpp"

NS_RESTD

// hash<shared_ptr<_Type>>
//   struct: hashes the STORED pointer, agreeing with operator==.
template<typename _Type>
struct hash<shared_ptr<_Type> >
{
    typedef shared_ptr<_Type> argument_type;
    typedef size_t            result_type;

    size_t operator()(const shared_ptr<_Type>& value) const D_NOEXCEPT
    {
        return hash<typename shared_ptr<_Type>::element_type*>()(value.get());
    }
};

// hash<unique_ptr<_Type, _Deleter>>
//   struct: hashes the stored pointer.
template<typename _Type, typename _Deleter>
struct hash<unique_ptr<_Type, _Deleter> >
{
    typedef unique_ptr<_Type, _Deleter> argument_type;
    typedef size_t                      result_type;

    size_t operator()(const unique_ptr<_Type, _Deleter>& value) const
    {
        return hash<typename unique_ptr<_Type, _Deleter>::pointer>()(
            value.get());
    }
};

NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_SMART_POINTER_HASH_
