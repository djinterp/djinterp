/*******************************************************************************
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
* path:      /inc/re_std/bitset/bitset_hash.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BITSET_BITSET_HASH_HPP
#define RE_STD_BITSET_BITSET_HASH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>

// re_std
#include "./bitset.hpp"
#include "../functional/hash.hpp"


namespace re_std
{


// ===========================================================================
// I.   HASH<BITSET>
// ===========================================================================

// hash<bitset<N>>
//   class: mixes the set bits. Relies on bitset's trimming invariant --
// see the header note.
template<std::size_t N>
struct hash< bitset<N> >
{
    std::size_t
    operator()(
        const bitset<N>& _b
    ) const
    {
        std::size_t _seed = N;
        for (std::size_t _i = 0; _i < N; ++_i)
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


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BITSET_BITSET_HASH_HPP
