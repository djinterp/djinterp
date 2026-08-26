/***********************************************************************
* re_std                                               allocate_shared.hpp
*
* allocator-aware single-allocation factory for shared_ptr.
*
* overloads provided:
*   allocate_shared<_T>(alloc, _args...)
*   allocate_shared<_T[]>(alloc, _n)              value-init
*   allocate_shared<_T[]>(alloc, _n, _u)          copy-init from _u
*   allocate_shared<_T[_N]>(alloc)                bounded, value-init
*   allocate_shared<_T[_N]>(alloc, _u)            bounded, copy-init
*
* the array forms rebind the allocator to unsigned char and allocate
* a single byte block large enough to hold the cb plus n elements,
* properly aligned for both. The cb's destroy() releases the block
* via the same allocator.
*
* see allocate_shared_for_overwrite.hpp for the default-init variants.
*
*
* path:      /inc/djinterp/re_std/memory/allocate_shared.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_ALLOCATE_SHARED_
#define DJINTERP_RE_STD_MEMORY_ALLOCATE_SHARED_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <new>

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/allocator_traits.hpp"
    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/memory/make_shared.hpp"           // for array_extent
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_bounded_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"
    #include "re_std/utility/forward.hpp"


namespace re_std
{

// ---------------------------------------------------------------------
// allocate_shared<_T>(alloc, _args...)  -  non-array form
// ---------------------------------------------------------------------
template<typename _T, typename _Alloc, typename... _Args>
typename enable_if
<
    !is_array<_T>::value,
    shared_ptr<_T>
>::type
allocate_shared(const _Alloc& _alloc, _Args&&... _args)
{
    typedef internal::sp_cb_alloc_inplace<_T, _Alloc>     cb_t;
    typedef typename allocator_traits<_Alloc>
        ::template rebind_alloc<cb_t>                     alloc_cb_t;
    typedef allocator_traits<alloc_cb_t>                  cb_traits;

    alloc_cb_t _a_cb(_alloc);
    cb_t* _cb = cb_traits::allocate(_a_cb, 1);

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            cb_traits::construct(_a_cb, _cb, _alloc,
                                 re_std::forward<_Args>(_args)...);
        }
        catch (...)
        {
            cb_traits::deallocate(_a_cb, _cb, 1);
            throw;
        }
    #else
        cb_traits::construct(_a_cb, _cb, _alloc,
                             re_std::forward<_Args>(_args)...);
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_cb->get(), _cb);
}


// All array forms share this internal byte-allocation + element-loop
// pattern. The two control points are:
//   - the count (runtime for unbounded, compile-time for bounded)
//   - the per-element construction expression (value-init or copy-init)
// We expand the four overloads inline rather than abstract a helper —
// the loop body has different captured types and the expansion is
// readable.

// ---------------------------------------------------------------------
// allocate_shared<_T[]>(alloc, _n)  -  unbounded, value-init
// ---------------------------------------------------------------------
template<typename _T, typename _Alloc>
typename enable_if
<
    is_unbounded_array<_T>::value,
    shared_ptr<_T>
>::type
allocate_shared(const _Alloc& _alloc, std::size_t _n)
{
    typedef typename remove_extent<_T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, _Alloc>        cb_t;
    typedef typename allocator_traits<_Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
                ::new (static_cast<void*>(_arr + _i)) _U();
        }
        catch (...)
        {
            while (_i > 0) { --_i; _arr[_i].~_U(); }
            if (_cb) _cb->~cb_t();
            byte_traits::deallocate(_ba, _mem, _bytes);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_alloc, _n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
            ::new (static_cast<void*>(_arr + _i)) _U();
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// allocate_shared<_T[]>(alloc, _n, _u)  -  unbounded, copy-init
// ---------------------------------------------------------------------
template<typename _T, typename _Alloc>
typename enable_if
<
    is_unbounded_array<_T>::value,
    shared_ptr<_T>
>::type
allocate_shared(const _Alloc& _alloc, std::size_t _n,
                const typename remove_extent<_T>::type& _u)
{
    typedef typename remove_extent<_T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, _Alloc>        cb_t;
    typedef typename allocator_traits<_Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
                ::new (static_cast<void*>(_arr + _i)) _U(_u);
        }
        catch (...)
        {
            while (_i > 0) { --_i; _arr[_i].~_U(); }
            if (_cb) _cb->~cb_t();
            byte_traits::deallocate(_ba, _mem, _bytes);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_alloc, _n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
            ::new (static_cast<void*>(_arr + _i)) _U(_u);
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// allocate_shared<_T[_N]>(alloc)  -  bounded, value-init
// ---------------------------------------------------------------------
template<typename _T, typename _Alloc>
typename enable_if
<
    is_bounded_array<_T>::value,
    shared_ptr<_T>
>::type
allocate_shared(const _Alloc& _alloc)
{
    typedef typename remove_extent<_T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, _Alloc>        cb_t;
    typedef typename allocator_traits<_Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    const std::size_t _n     = internal::array_extent<_T>::value;
    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
                ::new (static_cast<void*>(_arr + _i)) _U();
        }
        catch (...)
        {
            while (_i > 0) { --_i; _arr[_i].~_U(); }
            if (_cb) _cb->~cb_t();
            byte_traits::deallocate(_ba, _mem, _bytes);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_alloc, _n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
            ::new (static_cast<void*>(_arr + _i)) _U();
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// allocate_shared<_T[_N]>(alloc, _u)  -  bounded, copy-init
// ---------------------------------------------------------------------
template<typename _T, typename _Alloc>
typename enable_if
<
    is_bounded_array<_T>::value,
    shared_ptr<_T>
>::type
allocate_shared(const _Alloc& _alloc,
                const typename remove_extent<_T>::type& _u)
{
    typedef typename remove_extent<_T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, _Alloc>        cb_t;
    typedef typename allocator_traits<_Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    const std::size_t _n     = internal::array_extent<_T>::value;
    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
                ::new (static_cast<void*>(_arr + _i)) _U(_u);
        }
        catch (...)
        {
            while (_i > 0) { --_i; _arr[_i].~_U(); }
            if (_cb) _cb->~cb_t();
            byte_traits::deallocate(_ba, _mem, _bytes);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_alloc, _n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
            ::new (static_cast<void*>(_arr + _i)) _U(_u);
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_ALLOCATE_SHARED_
