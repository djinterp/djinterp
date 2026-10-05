/*******************************************************************************
* djinterp [re_std]                                       const_pointer_cast.hpp
*
* const_pointer_cast function header:
* shared_ptr cast that uses const_cast on the underlying pointer.
* Used to drop or add cv-qualification on the element type while
* sharing ownership with the source shared_ptr.
*
*
* path:      /inc/re_std/memory/const_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_CONST_POINTER_CAST_HPP
#define RE_STD_MEMORY_CONST_POINTER_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename T, typename U>
shared_ptr<T> const_pointer_cast(const shared_ptr<U>& _r) RE_STD_NOEXCEPT
{
    typedef typename shared_ptr<T>::element_type _E;
    return shared_ptr<T>(_r, const_cast<_E*>(_r.get()));
}

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    template<typename T, typename U>
    shared_ptr<T> const_pointer_cast(shared_ptr<U>&& _r) RE_STD_NOEXCEPT
    {
        typedef typename shared_ptr<T>::element_type _E;
        _E* _p = const_cast<_E*>(_r.get());
        return shared_ptr<T>(re_std::move(_r), _p);
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_CONST_POINTER_CAST_HPP
