/***********************************************************************
* restd                                                    get_deleter.hpp
*
* extract a typed pointer to a shared_ptr's stored deleter:
*   _D* d = restd::get_deleter<_D>(_sp);
*
* returns a non-null pointer when:
*   - _sp owns an object via a control block that stores a deleter
*     (i.e. NOT make_shared / allocate_shared, which use type-erased
*     in-place storage with no separate deleter)
*   - the stored deleter's type is exactly _D (typeid match — bases /
*     derived deleters do not match)
*
* otherwise returns null. Never throws.
*
* requires:
*   <typeinfo> support (D_ENV_CPP98_HAS_TYPEINFO). If absent, this
*   header is empty — there's no way to compare deleter types without
*   typeid.
*
*
* path:      /inc/restd/memory/get_deleter.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_GET_DELETER_
#define RESTD_MEMORY_GET_DELETER_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER && D_ENV_CPP98_HAS_TYPEINFO

    #include <typeinfo>

    #include "restd/memory/shared_ptr.hpp"


namespace restd
{

template<typename _D, typename _T>
_D* get_deleter(const shared_ptr<_T>& _p) D_NOEXCEPT
{
    return static_cast<_D*>(_p._sp_internal_get_deleter(typeid(_D)));
}


}  // namespace restd

#endif  // C++11+ && typeinfo

#endif  // RESTD_MEMORY_GET_DELETER_
