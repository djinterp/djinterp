/*******************************************************************************
* djinterp [re_std]                            allocate_shared_for_overwrite.hpp
*
* default-init allocator-aware variant:
*   allocate_shared_for_overwrite<T>(alloc)
*   allocate_shared_for_overwrite<T[]>(alloc, _n)
*
* see make_shared_for_overwrite.hpp for the semantic distinction
* between default-init and value-init. This file pairs that semantic
* with allocator-supplied storage.
*
*
* path:      /inc/re_std/memory/allocate_shared_for_overwrite.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_ALLOCATE_SHARED_FOR_OVERWRITE_HPP
#define RE_STD_MEMORY_ALLOCATE_SHARED_FOR_OVERWRITE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <new>

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/allocator_traits.hpp"
    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/memory/make_shared.hpp"                        // for array_extent
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_bounded_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"


namespace re_std
{

// allocate_shared_for_overwrite<T>(alloc)  -  non-array form
template<typename T, typename Alloc>
typename enable_if
<
    !is_array<T>::value,
    shared_ptr<T>
>::type
allocate_shared_for_overwrite(const Alloc& _alloc)
{
    typedef internal::sp_cb_alloc_inplace<T, Alloc>     cb_t;
    typedef typename allocator_traits<Alloc>
        ::template rebind_alloc<cb_t>                     alloc_cb_t;
    typedef allocator_traits<alloc_cb_t>                  cb_traits;

    alloc_cb_t _a_cb(_alloc);
    cb_t* _cb = cb_traits::allocate(_a_cb, 1);

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            cb_traits::construct(_a_cb, _cb, _alloc,
                                 internal::sp_for_overwrite_t());
        }
        catch (...)
        {
            cb_traits::deallocate(_a_cb, _cb, 1);
            throw;
        }
    #else
        cb_traits::construct(_a_cb, _cb, _alloc,
                             internal::sp_for_overwrite_t());
    #endif

    return shared_ptr<T>::_sp_internal_from_cb(_cb->get(), _cb);
}


// allocate_shared_for_overwrite<T[]>(alloc, _n)  -  array form
template<typename T, typename Alloc>
typename enable_if
<
    is_unbounded_array<T>::value,
    shared_ptr<T>
>::type
allocate_shared_for_overwrite(const Alloc& _alloc, std::size_t _n)
{
    typedef typename remove_extent<T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, Alloc>        cb_t;
    typedef typename allocator_traits<Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
            {
                ::new (static_cast<void*>(_arr + _i)) _U;  // default-init
            }
        }
        catch (...)
        {
            while (_i > 0)
            {
                --_i;
                _arr[_i].~_U();
            }
            if (_cb)
            {
                _cb->~cb_t();
            }
            byte_traits::deallocate(_ba, _mem, _bytes);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_alloc, _n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
        {
            ::new (static_cast<void*>(_arr + _i)) _U;
        }
    #endif

    return shared_ptr<T>::_sp_internal_from_cb(_arr, _cb);
}


// allocate_shared_for_overwrite<T[_N]>(alloc)  -  bounded array
template<typename T, typename Alloc>
typename enable_if
<
    is_bounded_array<T>::value,
    shared_ptr<T>
>::type
allocate_shared_for_overwrite(const Alloc& _alloc)
{
    typedef typename remove_extent<T>::type                       _U;
    typedef internal::sp_cb_alloc_inplace_array<_U, Alloc>        cb_t;
    typedef typename allocator_traits<Alloc>
        ::template rebind_alloc<unsigned char>                     byte_alloc_t;
    typedef allocator_traits<byte_alloc_t>                         byte_traits;

    const std::size_t _n     = internal::array_extent<T>::value;
    byte_alloc_t      _ba(_alloc);
    const std::size_t _bytes = cb_t::total_bytes(_n);
    unsigned char*    _mem = byte_traits::allocate(_ba, _bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            _cb = ::new (_mem) cb_t(_alloc, _n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
                ::new (static_cast<void*>(_arr + _i)) _U;
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
            ::new (static_cast<void*>(_arr + _i)) _U;
    #endif

    return shared_ptr<T>::_sp_internal_from_cb(_arr, _cb);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_ALLOCATE_SHARED_FOR_OVERWRITE_HPP
