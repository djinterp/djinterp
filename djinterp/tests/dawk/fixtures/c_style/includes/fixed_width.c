/* fixed_width.c: Include Directives -- the fixed-width integers */
#include "./fixed_width.h"  // corresponding header
// std
#include <inttypes.h>  // PRIu32: positive
#include <stddef.h>    // size_t: negative
#include <stdint.h>    // uint32_t: positive
// djinterp
#include "../djinterp.h"  // D_INLINE
// re_std
#include "../../re_std/cstdint/dstdint.h"  // uint32_t: negative


int d_value = 0;
