/******************************************************************************
* djinterp [restd]                                                   shuffle.hpp
*
* shuffle algorithm header:
*   Permutes the elements of [_first, _last) uniformly at random using
* the Fisher-Yates (Durstenfeld) algorithm and a caller-supplied
* Uniform Random Bit Generator (URBG).
*
*   URBG REQUIREMENTS (per C++11 [rand.req.urng]):
*   - g.result_type      - unsigned integral typedef
*   - URBG::min()        - static, returns smallest possible output
*   - URBG::max()        - static, returns largest possible output
*   - g()                - returns a value in [URBG::min(), URBG::max()]
*   Any callable satisfying this contract is accepted; restd does NOT
*   require <random>. For C++11+ users the standard engines
*   (std::mt19937 et al.) work out of the box.
*
*   PORTABILITY:
*   - std::shuffle is C++11. restd back-ports to C++98 by relaxing the
*     URBG parameter from forwarding reference to lvalue reference;
*     callers on C++98 supply a named URBG object (no temporaries).
*   - constexpr in std from C++26; restd does NOT add constexpr (URBG
*     state mutation is the canonical constexpr-hostile operation).
*   - Requires RandomAccessIterator (uses indexed access via subtraction
*     and operator+).
*   - UNBIASED INDEX GENERATION: rejection sampling against the largest
*     multiple of the desired bound that fits in the URBG range. The
*     modulo-bias trap (g() % bound) is avoided.
*   - Assumes URBG::min() == 0 (true for every C++11 standard engine
*     and the overwhelming majority of custom URBGs); a non-zero min is
*     subtracted from each draw to normalise.
*
*
* path:      /inc/djinterp/re_std/algorithm/shuffle.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SHUFFLE_
#define DJINTERP_RESTD_ALGORITHM_SHUFFLE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"


NS_RESTD


// ===========================================================================
// I.   INTERNAL: UNIFORM INTEGER IN [0, BOUND)
// ===========================================================================

// _shuffle_uniform_index_
//   internal helper: returns an integer uniformly distributed in
// [0, _bound) using rejection sampling against _g. Assumes
// _URBG::min() == 0 (see header note); a non-zero min is folded out
// at the call site by subtraction.
// note: bound must be > 0. Caller is responsible for guarding _bound == 0.
template<typename _URBG>
typename _URBG::result_type
_shuffle_uniform_index_(
    _URBG&                         _g,
    typename _URBG::result_type    _bound
)
{
    typedef typename _URBG::result_type _R;

    const _R _u_min = _URBG::min();
    const _R _u_max = _URBG::max();

    // shift the URBG range to [0, span]
    const _R _span = _u_max - _u_min;  // M = size - 1

    // residue of (span + 1) modulo bound, computed without overflow:
    // (span + 1) mod B = (span mod B + 1) mod B
    const _R _residue = (_span % _bound + 1) % _bound;

    // accept x iff x <= span - residue (no rejection when residue == 0)
    const _R _max_accept = _span - _residue;

    _R _x;
    do
    {
        _x = _g() - _u_min;
    }
    while (_x > _max_accept);

    return _x % _bound;
}


// ===========================================================================
// II.  SHUFFLE
// ===========================================================================

// shuffle
//   function: permutes the elements of [_first, _last) uniformly at
// random using _g as the source of randomness. O(N) draws from _g.
template<typename _RandomIt,
         typename _URBG>
void
shuffle(
    _RandomIt _first,
    _RandomIt _last,
    _URBG&    _g
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;
    typedef typename _URBG::result_type                          _R;

    _Diff _n = _last - _first;
    if (_n <= 1)
    {
        return;
    }

    // Fisher-Yates: from the back, swap each element with a uniformly
    // random earlier (or self) position
    for (_Diff _i = _n - 1; _i > 0; --_i)
    {
        // _bound = _i + 1 in URBG result_type
        const _R _bound = static_cast<_R>(_i) + static_cast<_R>(1);
        const _R _j     = _shuffle_uniform_index_(_g, _bound);
        iter_swap(_first + _i, _first + static_cast<_Diff>(_j));
    }
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SHUFFLE_
