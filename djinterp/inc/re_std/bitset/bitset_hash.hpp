/******************************************************************************
* djinterp [re_std]                                              bitset_hash.hpp
*
* bitset hash support header:
*   re_std::hash specialisation for bitset.
*
*   Folds every storage word through the usual golden-ratio mix. This
* is well-defined precisely BECAUSE bitset maintains the trimming
* invariant -- the unused high bits of the last word are always zero,
* so two bitsets that compare equal always hash equal. Without that
* invariant, hashing the raw words would be wrong.
*
*   PORTABILITY:
*   std has had hash<bitset> since C++11; re_std matches.
*
*
* path:      /inc/djinterp/re_std/bitset/bitset_hash.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BITSET_BITSET_HASH_
#define DJINTERP_RE_STD_BITSET_BITSET_HASH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>

// djinterp
#include "./bitset.hpp"
#include "../functional/hash.hpp"


NS_RESTD


// ===========================================================================
// I.   HASH<BITSET>
// ===========================================================================

// hash<bitset<_N>>
//   class: mixes the set bits. Relies on bitset's trimming invariant --
// see the header note.
template<std::size_t _N>
struct hash< bitset<_N> >
{
    std::size_t
    operator()(
        const bitset<_N>& _b
    ) const
    {
        std::size_t _seed = _N;
        for (std::size_t _i = 0; _i < _N; ++_i)
        {
            if (_b[_i])
            {
                _seed ^= _i + static_cast<std::size_t>(0x9E3779B9u)
                            + (_seed << 6) + (_seed >> 2);
            }
        }
        return _seed;
    }
};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BITSET_BITSET_HASH_
