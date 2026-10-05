/*******************************************************************************
* djinterp [re_std]                                make_shared_for_overwrite.hpp
*
* default-init variant of make_shared:
*   make_shared_for_overwrite<T>()      non-array, default-init
*   make_shared_for_overwrite<T[]>(_n)  unbounded array, default-init
*
* default-init means `::new (p) T;` (no parens). For trivial types
* this leaves storage in an indeterminate state — the caller is
* expected to overwrite every byte before reading. For class types
* with a user-provided default ctor, default-init runs that ctor
* (same as value-init).
*
* this is the make_shared analogue of make_unique_for_overwrite.
* added in std C++20; re_std back-ports unconditionally to C++11+.
*
*
* path:      /inc/re_std/memory/make_shared_for_overwrite.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_HPP
#define RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <new>

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/memory/make_shared.hpp"                        // for array_extent
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_bounded_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"


namespace re_std
{

// make_shared_for_overwrite<T>()  -  non-array form
template<typename T>
typename enable_if
<
    !is_array<T>::value,
    shared_ptr<T>
>::type
make_shared_for_overwrite()
{
    typedef internal::sp_cb_inplace<T> cb_t;
    cb_t* _cb = new cb_t(internal::sp_for_overwrite_t());
    return shared_ptr<T>::_sp_internal_from_cb(_cb->get(), _cb);
}


// make_shared_for_overwrite<T[]>(_n)  -  array form
template<typename T>
typename enable_if
<
    is_unbounded_array<T>::value,
    shared_ptr<T>
>::type
make_shared_for_overwrite(std::size_t _n)
{
    typedef typename remove_extent<T>::type _U;
    typedef internal::sp_cb_inplace_array<_U> cb_t;

    const std::size_t _bytes = cb_t::total_bytes(_n);
    void* _mem = ::operator new(_bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            _cb = ::new (_mem) cb_t(_n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
            {
                // default-init: no parens.
                ::new (static_cast<void*>(_arr + _i)) _U;
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
            ::operator delete(_mem);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
        {
            ::new (static_cast<void*>(_arr + _i)) _U;
        }
    #endif

    return shared_ptr<T>::_sp_internal_from_cb(_arr, _cb);
}


// make_shared_for_overwrite<T[_N]>()  -  bounded array
template<typename T>
typename enable_if
<
    is_bounded_array<T>::value,
    shared_ptr<T>
>::type
make_shared_for_overwrite()
{
    typedef typename remove_extent<T>::type _U;
    typedef internal::sp_cb_inplace_array<_U> cb_t;

    const std::size_t _n = internal::array_extent<T>::value;
    const std::size_t _bytes = cb_t::total_bytes(_n);
    void* _mem = ::operator new(_bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            _cb = ::new (_mem) cb_t(_n);
            _arr = _cb->data();
            for (; _i < _n; ++_i)
            {
                ::new (static_cast<void*>(_arr + _i)) _U;  // default-init
            }
        }
        catch (...)
        {
            while (_i > 0) { --_i; _arr[_i].~_U(); }
            if (_cb) _cb->~cb_t();
            ::operator delete(_mem);
            throw;
        }
    #else
        _cb = ::new (_mem) cb_t(_n);
        _arr = _cb->data();
        for (; _i < _n; ++_i)
        {
            ::new (static_cast<void*>(_arr + _i)) _U;
        }
    #endif

    return shared_ptr<T>::_sp_internal_from_cb(_arr, _cb);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_HPP
