/*******************************************************************************
* djinterp [re_std]                                       smart_pointer_hash.hpp
*
* smart_pointer_hash support header:
*   hash<shared_ptr<T>> and hash<unique_ptr<T, D>>, each defined once in its
* own header (shared_ptr_hash.hpp, unique_ptr_hash.hpp); this header
* includes both. It used to define both itself as well, so a translation
* unit that included it beside either granular header redefined the
* specialisation.
*   BOTH HASH THE STORED POINTER, NOT THE POINTEE, and that is the only
* defensible choice: hashing the pointee would disagree with operator==, which
* compares pointers. Two shared_ptrs to equal-but-distinct objects are NOT
* equal and must not hash equally; two shared_ptrs to the SAME object are
* equal and do.
*   hash<shared_ptr> USES get(), NOT owner_before. That means two shared_ptrs
* that share ownership but hold different stored pointers - an aliasing
* constructor's product, say - hash differently, which again matches
* operator== rather than owner ordering. If you want owner identity, that is
* what owner_less and owner_hash are for.
*
*
* path:      /inc/re_std/memory/smart_pointer_hash.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_SMART_POINTER_HASH_HPP
#define RE_STD_MEMORY_SMART_POINTER_HASH_HPP 1

// re_std
#include "./shared_ptr_hash.hpp"  // hash<shared_ptr<T>>
#include "./unique_ptr_hash.hpp"  // hash<unique_ptr<T, D>>

#endif  // RE_STD_MEMORY_SMART_POINTER_HASH_HPP
