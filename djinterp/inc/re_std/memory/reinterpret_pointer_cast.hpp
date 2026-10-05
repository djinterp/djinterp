/*******************************************************************************
* djinterp [re_std]                                 reinterpret_pointer_cast.hpp
*
* reinterpret_pointer_cast function header:
* shared_ptr cast that uses reinterpret_cast on the underlying pointer.
* Use this when you need to bit-pattern-reinterpret a pointer (e.g.
* round-trip through void*) while keeping shared ownership.
*
* in std this was added in C++17. re_std back-ports unconditionally to
* C++11+, since the underlying machinery (the aliasing ctor) is
* available on every C++11+ tier.
*
*
* path:      /inc/re_std/memory/reinterpret_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_REINTERPRET_POINTER_CAST_HPP
#define RE_STD_MEMORY_REINTERPRET_POINTER_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename T, typename U>
shared_ptr<T> reinterpret_pointer_cast(const shared_ptr<U>& _r) RE_STD_NOEXCEPT
{
    typedef typename shared_ptr<T>::element_type _E;
    return shared_ptr<T>(_r, reinterpret_cast<_E*>(_r.get()));
}

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    template<typename T, typename U>
    shared_ptr<T> reinterpret_pointer_cast(shared_ptr<U>&& _r) RE_STD_NOEXCEPT
    {
        typedef typename shared_ptr<T>::element_type _E;
        _E* _p = reinterpret_cast<_E*>(_r.get());
        return shared_ptr<T>(re_std::move(_r), _p);
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_REINTERPRET_POINTER_CAST_HPP
