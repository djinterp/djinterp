/***********************************************************************
* re_std                                                    make_shared.hpp
*
* single-allocation factory for shared_ptr.
*
* overloads provided:
*   make_shared<_T>(_args...)         non-array, value-init from args
*   make_shared<_T[]>(_n)             unbounded array, value-init
*   make_shared<_T[]>(_n, _u)         unbounded array, copy-init from _u
*   make_shared<_T[_N]>()             bounded array, value-init
*   make_shared<_T[_N]>(_u)           bounded array, copy-init from _u
*
* see make_shared_for_overwrite.hpp for the default-init variants.
*
*
* path:      /inc/djinterp/re_std/memory/make_shared.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_MAKE_SHARED_
#define DJINTERP_RE_STD_MEMORY_MAKE_SHARED_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <new>

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_bounded_array.hpp"
    #include "re_std/type_traits/is_unbounded_array.hpp"
    #include "re_std/type_traits/remove_extent.hpp"
    #include "re_std/utility/forward.hpp"


namespace re_std
{
namespace internal
{

    // Extracts the extent N of an array type. Yields 0 for non-arrays
    // and unbounded arrays. Used by the bounded-make_shared overloads.
    template<typename _T>
    struct array_extent
    {
        static D_CONSTEXPR const std::size_t value = 0;
    };

    template<typename _T, std::size_t _N>
    struct array_extent<_T[_N]>
    {
        static D_CONSTEXPR const std::size_t value = _N;
    };

}  // namespace internal


// ---------------------------------------------------------------------
// make_shared<_T>(_args...)  -  non-array form
// ---------------------------------------------------------------------
template<typename _T, typename... _Args>
typename enable_if
<
    !is_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared(_Args&&... _args)
{
    typedef internal::sp_cb_inplace<_T> cb_t;
    cb_t* _cb = new cb_t(re_std::forward<_Args>(_args)...);
    return shared_ptr<_T>::_sp_internal_from_cb(_cb->get(), _cb);
}


// ---------------------------------------------------------------------
// make_shared<_T[]>(_n)  -  unbounded array, value-init
// ---------------------------------------------------------------------
template<typename _T>
typename enable_if
<
    is_unbounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared(std::size_t _n)
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
                ::new (static_cast<void*>(_arr + _i)) _U();
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
            ::new (static_cast<void*>(_arr + _i)) _U();
        }
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// make_shared<_T[]>(_n, _u)  -  unbounded array, copy-init from _u
// ---------------------------------------------------------------------
template<typename _T>
typename enable_if
<
    is_unbounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared(std::size_t _n, const typename remove_extent<_T>::type& _u)
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
                ::new (static_cast<void*>(_arr + _i)) _U(_u);  // copy-init
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
            ::new (static_cast<void*>(_arr + _i)) _U(_u);
        }
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// make_shared<_T[_N]>()  -  bounded array, value-init
// ---------------------------------------------------------------------
template<typename _T>
typename enable_if
<
    is_bounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared()
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
                ::new (static_cast<void*>(_arr + _i)) _U();
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
            ::new (static_cast<void*>(_arr + _i)) _U();
        }
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


// ---------------------------------------------------------------------
// make_shared<_T[_N]>(_u)  -  bounded array, copy-init from _u
// ---------------------------------------------------------------------
template<typename _T>
typename enable_if
<
    is_bounded_array<_T>::value,
    shared_ptr<_T>
>::type
make_shared(const typename remove_extent<_T>::type& _u)
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
                ::new (static_cast<void*>(_arr + _i)) _U(_u);
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
            ::new (static_cast<void*>(_arr + _i)) _U(_u);
        }
    #endif

    return shared_ptr<_T>::_sp_internal_from_cb(_arr, _cb);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_MAKE_SHARED_
