/*******************************************************************************
* djinterp [re_std]                                     dynamic_pointer_cast.hpp
*
* dynamic_pointer_cast function header:
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
* path:      /inc/re_std/memory/dynamic_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_DYNAMIC_POINTER_CAST_HPP
#define RE_STD_MEMORY_DYNAMIC_POINTER_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& _r) RE_STD_NOEXCEPT
{
    typedef typename shared_ptr<T>::element_type _E;
    if (_E* _p = dynamic_cast<_E*>(_r.get()))
    {
        return shared_ptr<T>(_r, _p);
    }
    return shared_ptr<T>();
}

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    template<typename T, typename U>
    shared_ptr<T> dynamic_pointer_cast(shared_ptr<U>&& _r) RE_STD_NOEXCEPT
    {
        typedef typename shared_ptr<T>::element_type _E;
        if (_E* _p = dynamic_cast<_E*>(_r.get()))
        {
            return shared_ptr<T>(re_std::move(_r), _p);
        }
        // Failure: leave _r untouched, return empty.
        return shared_ptr<T>();
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_DYNAMIC_POINTER_CAST_HPP
