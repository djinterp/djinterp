/***********************************************************************
* restd                                              adjacent_difference.hpp
*
* adjacent_difference(_first, _last, _d_first [, _op]) writes the
* sequence
*   d[0] = src[0]
*   d[i] = src[i] - src[i-1]   for i > 0
* into _d_first.
*
* with a custom op, replaces the subtraction. Note _op(b, a) — second
* arg is the EARLIER element, matching std::adjacent_difference.
*
* return value: iterator past the last destination written.
*
*
* path:      /inc/djinterp/re_std/numeric/adjacent_difference.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_ADJACENT_DIFFERENCE_
#define RESTD_NUMERIC_ADJACENT_DIFFERENCE_ 1

#include "djinterp.hpp"

#include "restd/iterator/iterator_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "restd/utility/move.hpp"
#endif


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

// Default-op overload (operator-).
template<typename _InputIt, typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt adjacent_difference
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;

    typedef typename iterator_traits<_InputIt>::value_type _T;

    _T _prev = *_first;
    *_d_first = _prev;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        _T _cur = *_first;
        // *d = cur - prev
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            *_d_first = _cur - restd::move(_prev);
            _prev = restd::move(_cur);
        #else
            *_d_first = _cur - _prev;
            _prev = _cur;
        #endif
    }
    return _d_first;
}

// Custom-op overload.
template<typename _InputIt, typename _OutputIt, typename _BinOp>
D_CONSTEXPR_CPP14 _OutputIt adjacent_difference
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _BinOp     _op
)
{
    if (_first == _last) return _d_first;

    typedef typename iterator_traits<_InputIt>::value_type _T;

    _T _prev = *_first;
    *_d_first = _prev;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        _T _cur = *_first;
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            *_d_first = _op(_cur, restd::move(_prev));
            _prev = restd::move(_cur);
        #else
            *_d_first = _op(_cur, _prev);
            _prev = _cur;
        #endif
    }
    return _d_first;
}


}  // namespace restd

#endif  // RESTD_NUMERIC_ADJACENT_DIFFERENCE_
