/*******************************************************************************
* djinterp [re_std]                                           partition_copy.hpp
*
* partition_copy algorithm header:
*   Copies each element of [_first, _last) into one of two output
* ranges: those satisfying _pred go to _d_first_true, the rest to
* _d_first_false. Returns a pair of iterators one past the last
* element written in each output.
*
*   PORTABILITY:
*   - std::partition_copy is C++11; re_std back-ports to C++98 (no
*     language blocker — predicate plus conditional output).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/partition_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PARTITION_COPY_HPP
#define RE_STD_ALGORITHM_PARTITION_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../utility/pair.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   PARTITION_COPY
// ===========================================================================

// partition_copy
//   function: copies each element of [_first, _last) into one of two
// output ranges based on _pred. Returns pair(end_of_true_out,
// end_of_false_out).
template<typename InputIt,
         typename OutputItTrue,
         typename OutputItFalse,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 pair<OutputItTrue, OutputItFalse>
partition_copy(
    InputIt        _first,
    InputIt        _last,
    OutputItTrue   _d_first_true,
    OutputItFalse  _d_first_false,
    Pred           _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            *_d_first_true = *_first;
            ++_d_first_true;
        }
        else
        {
            *_d_first_false = *_first;
            ++_d_first_false;
        }
    }

    return pair<OutputItTrue, OutputItFalse>(_d_first_true, _d_first_false);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PARTITION_COPY_HPP
