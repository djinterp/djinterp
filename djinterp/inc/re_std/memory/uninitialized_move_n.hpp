/*******************************************************************************
* djinterp [re_std]                                     uninitialized_move_n.hpp
*
* sized variant of uninitialized_move.
*
* return value:
*   pair<InputIt, ForwardIt> — the input iterator advanced _n
*   positions, and the destination past-the-end iterator.
*
*
* path:      /inc/re_std/memory/uninitialized_move_n.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_MOVE_N_HPP
#define RE_STD_MEMORY_UNINITIALIZED_MOVE_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER && RE_STD_HAS_HEADER_NEW

    // std
    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"
    #include "re_std/utility/move.hpp"
    #include "re_std/utility/pair.hpp"


namespace re_std
{

template<typename InputIt, typename Size, typename ForwardIt>
pair<InputIt, ForwardIt> uninitialized_move_n
(
    InputIt    _first,
    Size       _n,
    ForwardIt  _d_first
)
{
    typedef typename internal::iter_value<ForwardIt>::type _T;

    ForwardIt _current = _d_first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _n > 0; ++_first, (void)++_current, --_n)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _T(re_std::move(*_first));
            }
            return pair<InputIt, ForwardIt>(_first, _current);
        }
        catch (...)
        {
            for (; _d_first != _current; ++_d_first)
            {
                re_std::destroy_at(re_std::addressof(*_d_first));
            }
            throw;
        }
    #else
        for (; _n > 0; ++_first, (void)++_current, --_n)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _T(re_std::move(*_first));
        }
        return pair<InputIt, ForwardIt>(_first, _current);
    #endif
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER && RE_STD_HAS_HEADER_NEW

#endif  // RE_STD_MEMORY_UNINITIALIZED_MOVE_N_HPP
