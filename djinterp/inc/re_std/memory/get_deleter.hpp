/*******************************************************************************
* djinterp [re_std]                                              get_deleter.hpp
*
* extract a typed pointer to a shared_ptr's stored deleter:
*   D* d = re_std::get_deleter<D>(_sp);
*
* returns a non-null pointer when:
*   - _sp owns an object via a control block that stores a deleter
*     (i.e. NOT make_shared / allocate_shared, which use type-erased
*     in-place storage with no separate deleter)
*   - the stored deleter's type is exactly D (typeid match — bases /
*     derived deleters do not match)
*
* otherwise returns null. Never throws.
*
* requires:
*   <typeinfo> support (RE_STD_HAS_RTTI). If absent, this
*   header is empty — there's no way to compare deleter types without
*   typeid.
*
*
* path:      /inc/re_std/memory/get_deleter.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_GET_DELETER_HPP
#define RE_STD_MEMORY_GET_DELETER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER && RE_STD_HAS_RTTI

    // std
    #include <typeinfo>

    #include "re_std/memory/shared_ptr.hpp"


namespace re_std
{

template<typename D, typename T>
D* get_deleter(const shared_ptr<T>& _p) RE_STD_NOEXCEPT
{
    return static_cast<D*>(_p._sp_internal_get_deleter(typeid(D)));
}


}  // re_std
#endif  // C++11+ && typeinfo

#endif  // RE_STD_MEMORY_GET_DELETER_HPP
