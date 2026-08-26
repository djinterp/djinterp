/***********************************************************************
* re_std                                      make_shared_for_overwrite.hpp
*
* default-init variant of make_shared:
*   make_shared_for_overwrite<_T>()      non-array, default-init
*   make_shared_for_overwrite<_T[]>(_n)  unbounded array, default-init
*
* default-init means `::new (p) _T;` (no parens). For trivial types
* this leaves storage in an indeterminate state — the caller is
* expected to overwrite every byte before reading. For class types
* with a user-provided default ctor, default-init runs that ctor
* (same as value-init).
*
* this is the make_shared analogue of make_unique_for_overwrite.
* added in std C++20; re_std back-ports unconditionally to C++11+.
*
*
* path:      /inc/djinterp/re_std/memory/make_shared_for_overwrite.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_
#define DJINTERP_RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <new>

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/memory/make_shared.hpp"               // for array_extent
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_bounded_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"


namespace re_std
{

// make_shared_for_overwrite<_T>()  -  non-array form
template<typename _T>
typename enable_if
<
    !is_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared_for_overwrite()
{
    typedef internal::sp_cb_inplace<_T> cb_t;
    cb_t* _cb = new cb_t(internal::sp_for_overwrite_t());
    return shared_ptr<_T>::_sp_internal_from_cb(_cb->get(), _cb);
}


// make_shared_for_overwrite<_T[]>(_n)  -  array form
template<typename _T>
typename enable_if
<
    is_unbounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared_for_overwrite(std::size_t _n)
{
    typedef typename remove_extent<_T>::type _U;
    typedef internal::sp_cb_inplace_array<_U> cb_t;

    const std::size_t _bytes = cb_t::total_bytes(_n);
    void* _mem = ::operator new(_bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
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

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// make_shared_for_overwrite<_T[_N]>()  -  bounded array
template<typename _T>
typename enable_if
<
    is_bounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared_for_overwrite()
{
    typedef typename remove_extent<_T>::type _U;
    typedef internal::sp_cb_inplace_array<_U> cb_t;

    const std::size_t _n = internal::array_extent<_T>::value;
    const std::size_t _bytes = cb_t::total_bytes(_n);
    void* _mem = ::operator new(_bytes);

    cb_t*       _cb  = 0;
    _U*         _arr = 0;
    std::size_t _i   = 0;

    #if D_ENV_CPP98_HAS_EXCEPTION
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

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_MAKE_SHARED_FOR_OVERWRITE_
