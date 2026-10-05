/*******************************************************************************
* djinterp [re_std]                                                   rotate.hpp
*
* rotate algorithm header:
*   Performs a left rotation on [_first, _last) such that the element
* originally at _middle becomes the new beginning. Returns the iterator
* to the position where the element originally at _first now sits,
* i.e. _first + (_last - _middle).
*
*   PORTABILITY:
*   - std::rotate is C++98 but returned void until C++11. re_std ships
*     the C++11 (iterator-returning) signature on every tier.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Algorithm: the forward-iterator-only swap-walk from
*     [Stepanov & McJones, "Elements of Programming"]. The natural form
*     is tail-recursive; re_std converts to an outer while(true) loop to
*     avoid relying on the compiler's TCO.
*
*
* path:      /inc/re_std/algorithm/rotate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ROTATE_HPP
#define RE_STD_ALGORITHM_ROTATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   ROTATE
// ===========================================================================

// rotate
//   function: left-rotates [_first, _last) so that the element at
// _middle becomes the new beginning. Returns the iterator pointing to
// the new position of the element formerly at _first
// (= _first + (_last - _middle)).
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
rotate(
    ForwardIt _first,
    ForwardIt _middle,
    ForwardIt _last
)
{
    if (_first == _middle)
    {
        return _last;
    }
    if (_middle == _last)
    {
        return _first;
    }

    ForwardIt _result      = _first;
    bool       _result_set  = false;

    // outer loop: manually-eliminated tail recursion. Each iteration
    // rotates [_first, _last) by _middle; the residual rotation needed
    // for the suffix becomes the next iteration's input.
    while (true)
    {
        if (_first == _middle)
        {
            return _result;
        }
        if (_middle == _last)
        {
            return _result;
        }

        ForwardIt _write     = _first;
        ForwardIt _next_read = _first;  // tracks "unread" boundary
        ForwardIt _read      = _middle;

        while (_read != _last)
        {
            if (_write == _next_read)
            {
                _next_read = _read;
            }
            iter_swap(_write, _read);
            ++_write;
            ++_read;
        }

        // After the first pass, _write points to the new position of
        // the element formerly at _first. Capture it once.
        if (!_result_set)
        {
            _result     = _write;
            _result_set = true;
        }

        // tail-recurse on the suffix that still needs rotating
        _first  = _write;
        _middle = _next_read;
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ROTATE_HPP
