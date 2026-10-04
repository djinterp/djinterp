/***********************************************************************
* re_std                                             const_pointer_cast.hpp
*
* shared_ptr cast that uses const_cast on the underlying pointer.
* Used to drop or add cv-qualification on the element type while
* sharing ownership with the source shared_ptr.
*
*
* path:      /inc/djinterp/re_std/memory/const_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_CONST_POINTER_CAST_
#define DJINTERP_RE_STD_MEMORY_CONST_POINTER_CAST_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename _T, typename _U>
shared_ptr<_T> const_pointer_cast(const shared_ptr<_U>& _r) D_NOEXCEPT
{
    typedef typename shared_ptr<_T>::element_type _E;
    return shared_ptr<_T>(_r, const_cast<_E*>(_r.get()));
}

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    template<typename _T, typename _U>
    shared_ptr<_T> const_pointer_cast(shared_ptr<_U>&& _r) D_NOEXCEPT
    {
        typedef typename shared_ptr<_T>::element_type _E;
        _E* _p = const_cast<_E*>(_r.get());
        return shared_ptr<_T>(re_std::move(_r), _p);
    }

#endif


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_CONST_POINTER_CAST_
