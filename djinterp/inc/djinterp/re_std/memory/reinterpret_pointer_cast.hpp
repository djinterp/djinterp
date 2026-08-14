/***********************************************************************
* restd                                         reinterpret_pointer_cast.hpp
*
* shared_ptr cast that uses reinterpret_cast on the underlying pointer.
* Use this when you need to bit-pattern-reinterpret a pointer (e.g.
* round-trip through void*) while keeping shared ownership.
*
* in std this was added in C++17. restd back-ports unconditionally to
* C++11+, since the underlying machinery (the aliasing ctor) is
* available on every C++11+ tier.
*
*
* path:      /inc/djinterp/re_std/memory/reinterpret_pointer_cast.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_REINTERPRET_POINTER_CAST_
#define RESTD_MEMORY_REINTERPRET_POINTER_CAST_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"
    #include "restd/utility/move.hpp"


namespace restd
{

template<typename _T, typename _U>
shared_ptr<_T> reinterpret_pointer_cast(const shared_ptr<_U>& _r) D_NOEXCEPT
{
    typedef typename shared_ptr<_T>::element_type _E;
    return shared_ptr<_T>(_r, reinterpret_cast<_E*>(_r.get()));
}

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    template<typename _T, typename _U>
    shared_ptr<_T> reinterpret_pointer_cast(shared_ptr<_U>&& _r) D_NOEXCEPT
    {
        typedef typename shared_ptr<_T>::element_type _E;
        _E* _p = reinterpret_cast<_E*>(_r.get());
        return shared_ptr<_T>(restd::move(_r), _p);
    }

#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_REINTERPRET_POINTER_CAST_
