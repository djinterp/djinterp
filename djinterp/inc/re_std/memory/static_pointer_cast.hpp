/*******************************************************************************
* djinterp [re_std]                                      static_pointer_cast.hpp
*
* static_pointer_cast function header:
* shared_ptr cast that uses static_cast on the underlying pointer.
* Use when the conversion is known safe at compile time (e.g. unrelated
* but compatible types, or down-cast in a hierarchy you know is the
* right way round).
*
* The result aliases the source's control block via the aliasing ctor,
* so both shared_ptrs share ownership: destroying the result decrements
* the same use_count as destroying the source.
*
*
* path:      /inc/re_std/memory/static_pointer_cast.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_STATIC_POINTER_CAST_HPP
#define RE_STD_MEMORY_STATIC_POINTER_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

// Const-ref overload — always available.
template<typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& _r) RE_STD_NOEXCEPT
{
    typedef typename shared_ptr<T>::element_type _E;
    return shared_ptr<T>(_r, static_cast<_E*>(_r.get()));
}

// Rvalue overload — std added in C++20; re_std offers it whenever
// rvalue references are available, since the underlying machinery
// (rvalue aliasing ctor) is the same on every C++11+ tier.
#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    template<typename T, typename U>
    shared_ptr<T> static_pointer_cast(shared_ptr<U>&& _r) RE_STD_NOEXCEPT
    {
        typedef typename shared_ptr<T>::element_type _E;
        _E* _p = static_cast<_E*>(_r.get());
        return shared_ptr<T>(re_std::move(_r), _p);
    }

#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_STATIC_POINTER_CAST_HPP
