/*******************************************************************************
* djinterp [re_std]                                          type_index_hash.hpp
*
* type_index_hash support header:
*   hash<type_index> specialisation.
*
*   Delegates straight to type_index::hash_code(), which forwards to
* type_info::hash_code() - the value the C++ ABI already computes and
* guarantees is equal for equal types.  Hashing anything else here (the name
* string, say) would be both slower and WRONG on implementations where two
* type_info objects for the same type can have distinct addresses across
* shared-library boundaries but still compare equal; hash_code() is the only
* thing specified to agree with operator==.
*
*   STD IS C++11; re_std IS C++11 - inherits type_index's own floor.
*
*
* path:      /inc/re_std/typeindex/type_index_hash.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPEINDEX_TYPE_INDEX_HASH_HPP
#define RE_STD_TYPEINDEX_TYPE_INDEX_HASH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// hash<type_index> exists where type_index does: C++11, with RTTI
#if ( (RE_STD_LANG_IS_CPP11_OR_HIGHER) &&                                      \
      (RE_STD_HAS_RTTI) )

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./type_index.hpp"

namespace re_std
{

// hash<type_index>
//   struct: hash support for type_index.
template<>
struct hash<type_index>
{
    typedef type_index argument_type;
    typedef size_t     result_type;

    size_t operator()(const type_index& value) const RE_STD_NOEXCEPT
    {
        return value.hash_code();
    }
};

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPEINDEX_TYPE_INDEX_HASH_HPP
