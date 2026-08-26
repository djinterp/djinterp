/******************************************************************************
* re_std [typeindex]                                        type_index_hash.hpp
*
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
* path:      /inc/djinterp/re_std/typeindex/type_index_hash.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPEINDEX_TYPE_INDEX_HASH_
#define DJINTERP_RE_STD_TYPEINDEX_TYPE_INDEX_HASH_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./type_index.hpp"

NS_RESTD

// hash<type_index>
//   struct: hash support for type_index.
template<>
struct hash<type_index>
{
    typedef type_index argument_type;
    typedef size_t     result_type;

    size_t operator()(const type_index& value) const D_NOEXCEPT
    {
        return value.hash_code();
    }
};

NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPEINDEX_TYPE_INDEX_HASH_
