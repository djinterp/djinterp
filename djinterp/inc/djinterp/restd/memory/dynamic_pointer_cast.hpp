/***********************************************************************
* restd                                            dynamic_pointer_cast.hpp
*
* shared_ptr cast that uses dynamic_cast on the underlying pointer.
* When the cast fails, the result is an empty shared_ptr.
*
* rvalue overload semantics ([util.smartptr.shared.cast]):
*   On success, ownership is transferred from `r` to the result. On
*   failure, `r` is LEFT UNCHANGED — only successful casts consume the
*   rvalue. This means we must check the cast first, then call the
*   rvalue aliasing ctor only if it succeeded.
*
* requires:
*   The pointee type must be polymorphic (have at least one virtual
*   function) for dynamic_cast to work. This is a runtime requirement
*   inherited from C++ itself, not specific to restd.
*
*
* path:      /inc/restd/memory/dynamic_pointer_cast.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_DYNAMIC_POINTER_CAST_
#define RESTD_MEMORY_DYNAMIC_POINTER_CAST_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"
    #include "restd/utility/move.hpp"


namespace restd
{

template<typename _T, typename _U>
shared_ptr<_T> dynamic_pointer_cast(const shared_ptr<_U>& _r) D_NOEXCEPT
{
    typedef typename shared_ptr<_T>::element_type _E;
    if (_E* _p = dynamic_cast<_E*>(_r.get()))
    {
        return shared_ptr<_T>(_r, _p);
    }
    return shared_ptr<_T>();
}

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    template<typename _T, typename _U>
    shared_ptr<_T> dynamic_pointer_cast(shared_ptr<_U>&& _r) D_NOEXCEPT
    {
        typedef typename shared_ptr<_T>::element_type _E;
        if (_E* _p = dynamic_cast<_E*>(_r.get()))
        {
            return shared_ptr<_T>(restd::move(_r), _p);
        }
        // Failure: leave _r untouched, return empty.
        return shared_ptr<_T>();
    }

#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_DYNAMIC_POINTER_CAST_
