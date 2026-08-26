/***********************************************************************
* re_std                                           dynamic_pointer_cast.hpp
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
*   inherited from C++ itself, not specific to re_std.
*
*
* path:      /inc/djinterp/re_std/memory/dynamic_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_DYNAMIC_POINTER_CAST_
#define DJINTERP_RE_STD_MEMORY_DYNAMIC_POINTER_CAST_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
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
            return shared_ptr<_T>(re_std::move(_r), _p);
        }
        // Failure: leave _r untouched, return empty.
        return shared_ptr<_T>();
    }

#endif


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_DYNAMIC_POINTER_CAST_
