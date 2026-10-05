/*******************************************************************************
* djinterp [c]                                                    event_common.c
*
* Event foundations -- tier 0 definitions.
*   Defines the one out-of-line function event_common.h declares: the prime
* sizing of bucket arrays. Everything else in that header is inline.
*
* path:      /src/djinterp/c/event/event_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX


/*
d_event_next_prime
  Trial division by odd factors up to the square root: bucket counts are small
enough that nothing cleverer pays for itself, and `factor <= candidate /
factor` bounds the loop without the overflow `factor * factor` could hit. The
search stops rather than wraps; when no prime at or above _n fits in a size_t
the result is 0, which no caller can mistake for a bucket count.
*/
size_t
d_event_next_prime(
    size_t _n
)
{
    // the two smallest primes answer every input up to them
    if (_n <= 2u)
    {
        return 2u;
    }

    // SIZE_MAX is odd, so stepping an even _n up by one cannot wrap
    size_t candidate = ((_n % 2u) == 0u) ? (_n + 1u) : _n;

    // walk the odd numbers until one is prime or size_t runs out
    for (;;)
    {
        bool is_prime = true;

        // trial division by odd factors while factor^2 <= candidate
        for (size_t factor = 3u; factor <= (candidate / factor); factor += 2u)
        {
            if ((candidate % factor) == 0u)
            {
                is_prime = false;

                break;
            }
        }

        if (is_prime)
        {
            return candidate;
        }

        // the next odd number would wrap: no prime at or above _n fits
        if (candidate > (SIZE_MAX - 2u))
        {
            break;
        }

        candidate += 2u;
    }

    return 0u;
}
