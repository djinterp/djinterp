/***********************************************************************
* restd                                              static_pointer_cast.hpp
*
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
* path:      /inc/restd/memory/static_pointer_cast.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_STATIC_POINTER_CAST_
#define RESTD_MEMORY_STATIC_POINTER_CAST_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"
    #include "restd/utility/move.hpp"


namespace restd
{

// Const-ref overload — always available.
template<typename _T, typename _U>
shared_ptr<_T> static_pointer_cast(const shared_ptr<_U>& _r) D_NOEXCEPT
{
    typedef typename shared_ptr<_T>::element_type _E;
    return shared_ptr<_T>(_r, static_cast<_E*>(_r.get()));
}

// Rvalue overload — std added in C++20; restd offers it whenever
// rvalue references are available, since the underlying machinery
// (rvalue aliasing ctor) is the same on every C++11+ tier.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    template<typename _T, typename _U>
    shared_ptr<_T> static_pointer_cast(shared_ptr<_U>&& _r) D_NOEXCEPT
    {
        typedef typename shared_ptr<_T>::element_type _E;
        _E* _p = static_cast<_E*>(_r.get());
        return shared_ptr<_T>(restd::move(_r), _p);
    }

#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_STATIC_POINTER_CAST_
