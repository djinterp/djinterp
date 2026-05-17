/******************************************************************************
* djinterp [restd]                                            partition_copy.hpp
*
* partition_copy algorithm header:
*   Copies each element of [_first, _last) into one of two output
* ranges: those satisfying _pred go to _d_first_true, the rest to
* _d_first_false. Returns a pair of iterators one past the last
* element written in each output.
*
*   PORTABILITY:
*   - std::partition_copy is C++11; restd back-ports to C++98 (no
*     language blocker — predicate plus conditional output).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/partition_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_PARTITION_COPY_
#define DJINTERP_RESTD_ALGORITHM_PARTITION_COPY_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "../utility/pair.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   PARTITION_COPY
// ===========================================================================

// partition_copy
//   function: copies each element of [_first, _last) into one of two
// output ranges based on _pred. Returns pair(end_of_true_out,
// end_of_false_out).
template<typename _InputIt,
         typename _OutputItTrue,
         typename _OutputItFalse,
         typename _Pred>
D_CONSTEXPR_CPP14 pair<_OutputItTrue, _OutputItFalse>
partition_copy(
    _InputIt        _first,
    _InputIt        _last,
    _OutputItTrue   _d_first_true,
    _OutputItFalse  _d_first_false,
    _Pred           _pred
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

    return pair<_OutputItTrue, _OutputItFalse>(_d_first_true, _d_first_false);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_PARTITION_COPY_
