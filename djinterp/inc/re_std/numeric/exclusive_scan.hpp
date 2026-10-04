/***********************************************************************
* re_std                                                   exclusive_scan.hpp
*
* exclusive_scan writes the running fold up to but EXCLUDING the
* corresponding input:
*   d[0] = init
*   d[1] = op(init, src[0])
*   d[i] = op(d[i-1], src[i-1])
*
* the standard requires init be passed explicitly — there is no
* default-init overload on exclusive_scan.
*
* op must be associative (parallel-friendly). serial here.
*
* return value: iterator past the last destination written.
*
*
* path:      /inc/djinterp/re_std/numeric/exclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_EXCLUSIVE_SCAN_
#define DJINTERP_RE_STD_NUMERIC_EXCLUSIVE_SCAN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

// Default-op (operator+).
template<typename _InputIt, typename _OutputIt, typename _T>
D_CONSTEXPR_CPP14 _OutputIt exclusive_scan
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _T         _init
)
{
    while (_first != _last)
    {
        _T _next = _init + *_first;
        *_d_first = re_std::move(_init);
        _init = re_std::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}

// Custom-op overload.
template<typename _InputIt, typename _OutputIt, typename _T, typename _BinOp>
D_CONSTEXPR_CPP14 _OutputIt exclusive_scan
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _T         _init,
    _BinOp     _op
)
{
    while (_first != _last)
    {
        _T _next = _op(_init, *_first);
        *_d_first = re_std::move(_init);
        _init = re_std::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_NUMERIC_EXCLUSIVE_SCAN_
