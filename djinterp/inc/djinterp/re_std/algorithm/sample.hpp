/******************************************************************************
* djinterp [re_std]                                                   sample.hpp
*
* sample algorithm header:
*   Selects _n elements from [_first, _last) uniformly at random and
* writes them to _out using a caller-supplied Uniform Random Bit
* Generator (URBG). Returns the iterator one past the last element
* written.
*
*   ALGORITHM SELECTION (tag-dispatched on the input iterator category):
*   - ForwardIterator (or stronger): Algorithm S [Knuth, TAOCP 3.4.2].
*     Single-pass selection sampling that consults the population's
*     remaining size; output order is the input order; copies exactly
*     min(n, |population|) elements.
*   - InputIterator only:            Algorithm R [Vitter, ACM TOMS 1985].
*     Single-pass reservoir sampling; output order is NOT the input
*     order (per std::sample's contract). Requires the output to be a
*     RandomAccessIterator to overwrite reservoir slots.
*   The std::sample contract matches this split exactly.
*
*   URBG REQUIREMENTS:
*   Same as shuffle (see shuffle.hpp). Re_std does not require <random>.
*
*   PORTABILITY:
*   - std::sample is C++17; re_std back-ports to C++98 with lvalue-ref
*     URBG (callers supply a named URBG object).
*   - C++11+ uses move when writing into the output for the reservoir
*     variant; C++98 uses copy.
*   - Not constexpr (URBG state is mutated; mirrors shuffle).
*   - UNBIASED random integer generation: same rejection-sampling
*     helper as shuffle (inlined here to avoid coupling the two
*     headers).
*
*
* path:      /inc/djinterp/re_std/algorithm/sample.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_SAMPLE_
#define DJINTERP_RE_STD_ALGORITHM_SAMPLE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "../iterator/iterator_traits.hpp"
#include "../iterator/input_iterator_tag.hpp"
#include "../iterator/forward_iterator_tag.hpp"


NS_RESTD


// ===========================================================================
// I.   INTERNAL: UNIFORM INTEGER IN [0, BOUND)
// ===========================================================================

// _sample_uniform_index_
//   internal helper: unbiased uniform integer in [0, _bound) via
// rejection sampling. Mirrors the helper in shuffle.hpp; inlined here
// to keep the two algorithms independent. See shuffle.hpp for the
// derivation comments.
template<typename _URBG>
typename _URBG::result_type
_sample_uniform_index_(
    _URBG&                         _g,
    typename _URBG::result_type    _bound
)
{
    typedef typename _URBG::result_type _R;

    const _R _u_min      = _URBG::min();
    const _R _span       = _URBG::max() - _u_min;
    const _R _residue    = (_span % _bound + 1) % _bound;
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
// II.  SAMPLE - FORWARD ITERATOR (ALGORITHM S)
// ===========================================================================

// _sample_impl_ (forward) - selection sampling
//   Knuth Algorithm S. Walks the population once and decides for each
// element whether to include it, based on its index and the
// already-selected count. Preserves input order in the output.
template<typename _PopIt,
         typename _SampleIt,
         typename _Distance,
         typename _URBG>
_SampleIt
_sample_impl_(
    _PopIt                  _first,
    _PopIt                  _last,
    _SampleIt               _out,
    _Distance               _n,
    _URBG&                  _g,
    forward_iterator_tag
)
{
    typedef typename _URBG::result_type _R;

    // population size in one pass (forward-iterable, so distance is
    // O(N) but allowed)
    _Distance _remaining = 0;
    for (_PopIt _it = _first; _it != _last; ++_it)
    {
        ++_remaining;
    }

    // clamp _n to the population size
    if (_n > _remaining)
    {
        _n = _remaining;
    }
    if (_n <= 0)
    {
        return _out;
    }

    // include each element with probability _n / _remaining
    while (_n > 0)
    {
        const _R _bound = static_cast<_R>(_remaining);
        const _R _pick  = _sample_uniform_index_(_g, _bound);
        if (_pick < static_cast<_R>(_n))
        {
            *_out = *_first;
            ++_out;
            --_n;
        }
        ++_first;
        --_remaining;
    }

    return _out;
}


// ===========================================================================
// III. SAMPLE - INPUT ITERATOR (ALGORITHM R)
// ===========================================================================

// _sample_impl_ (input) - reservoir sampling
//   Vitter Algorithm R. The output iterator must be a random-access
// iterator since we overwrite reservoir slots in place. The first _n
// elements are copied unconditionally; each subsequent element of
// index i replaces a random reservoir slot with probability _n / i.
template<typename _PopIt,
         typename _SampleIt,
         typename _Distance,
         typename _URBG>
_SampleIt
_sample_impl_(
    _PopIt                  _first,
    _PopIt                  _last,
    _SampleIt               _out,
    _Distance               _n,
    _URBG&                  _g,
    input_iterator_tag
)
{
    typedef typename _URBG::result_type _R;

    if (_n <= 0)
    {
        return _out;
    }

    // fill the reservoir with the first _n elements (or fewer if the
    // population is shorter)
    _Distance _k = 0;
    while ( (_k < _n) &&
            (_first != _last) )
    {
        _out[_k] = *_first;
        ++_k;
        ++_first;
    }

    // _k = min(_n, population_size); now stream the rest, replacing
    // reservoir slots with diminishing probability
    _Distance _i = _k;
    while (_first != _last)
    {
        ++_i;
        const _R _bound = static_cast<_R>(_i);
        const _R _pick  = _sample_uniform_index_(_g, _bound);
        if (_pick < static_cast<_R>(_n))
        {
            _out[static_cast<_Distance>(_pick)] = *_first;
        }
        ++_first;
    }

    return _out + _k;
}


// ===========================================================================
// IV.  SAMPLE - PUBLIC ENTRY
// ===========================================================================

// sample
//   function: writes min(_n, |[_first, _last)|) elements selected
// uniformly at random from [_first, _last) to _out, using _g as the
// source of randomness. Returns the iterator one past the last
// written element. Dispatches on the population's iterator category;
// see the file header for the algorithm-selection contract.
template<typename _PopIt,
         typename _SampleIt,
         typename _Distance,
         typename _URBG>
_SampleIt
sample(
    _PopIt    _first,
    _PopIt    _last,
    _SampleIt _out,
    _Distance _n,
    _URBG&    _g
)
{
    typedef typename iterator_traits<_PopIt>::iterator_category _Cat;
    return _sample_impl_(_first, _last, _out, _n, _g, _Cat());
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_SAMPLE_
